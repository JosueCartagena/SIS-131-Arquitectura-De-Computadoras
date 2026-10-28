/*
 * Simulador didáctico para terminal.asm  (versión en C)
 *   - RAM de 256 bytes, VRAM 4x4 en 0xF0..0xFF
 *   - Puerto 0x01 (estado) y 0x02 (datos) del teclado
 *   - Registros R0..R15 de 8 bits
 *
 * Compilar:  gcc -Wall -O2 -o simulador simulador.c
 * Uso:       ./simulador terminal.asm                  (interactivo, Esc sale)
 *            ./simulador terminal.asm "HOLA\b\bXY"     (demo automática)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#define MAX_INSTR 512
#define MAX_LABEL 128

typedef enum { OP_MOV, OP_LOAD, OP_STORE, OP_IN, OP_OUT, OP_ADD, OP_SUB, OP_AND,
               OP_CMP, OP_JZ, OP_JNZ, OP_JMP, OP_NOP, OP_HLT } Op;
typedef enum { K_NONE, K_REG, K_IMM, K_MEM_IMM, K_MEM_REG } Kind;

typedef struct { Kind k; int v; } Operando;
typedef struct { Op op; Operando a, b; char etq[48]; int linea; } Instr;
typedef struct { char nombre[48]; int pc; } Etiqueta;

static Instr prog[MAX_INSTR];
static int nprog = 0;
static Etiqueta etqs[MAX_LABEL];
static int netq = 0;

/* estado de la CPU */
static uint8_t mem[256], reg[16];
static int zf = 0, pc = 0;
static uint8_t p_estado = 0, p_dato = 0;   /* puertos 0x01 y 0x02 */

/* ---------- carga y parseo ---------- */
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = 0;
    return s;
}

static Operando parse_op(const char *t) {
    Operando o = { K_NONE, 0 };
    char buf[64];
    strncpy(buf, t, sizeof buf - 1); buf[sizeof buf - 1] = 0;
    char *s = trim(buf);
    int mem_ref = 0;
    if (*s == '[') {
        mem_ref = 1;
        s++;
        char *c = strchr(s, ']');
        if (c) *c = 0;
        s = trim(s);
    }
    if ((s[0] == 'R' || s[0] == 'r') && isdigit((unsigned char)s[1])) {
        o.v = atoi(s + 1);
        o.k = mem_ref ? K_MEM_REG : K_REG;
    } else {
        o.v = (int)(strtol(s, NULL, 0) & 0xFF);
        o.k = mem_ref ? K_MEM_IMM : K_IMM;
    }
    return o;
}

static int op_desde_texto(const char *m, Op *out) {
    static const struct { const char *n; Op o; } t[] = {
        {"MOV",OP_MOV},{"LOAD",OP_LOAD},{"STORE",OP_STORE},{"IN",OP_IN},{"OUT",OP_OUT},
        {"ADD",OP_ADD},{"SUB",OP_SUB},{"AND",OP_AND},{"CMP",OP_CMP},{"JZ",OP_JZ},
        {"JNZ",OP_JNZ},{"JMP",OP_JMP},{"NOP",OP_NOP},{"HLT",OP_HLT} };
    for (size_t i = 0; i < sizeof t / sizeof t[0]; i++)
        if (strcasecmp(m, t[i].n) == 0) { *out = t[i].o; return 1; }
    return 0;
}

static int cargar(const char *archivo) {
    FILE *f = fopen(archivo, "r");
    if (!f) { perror(archivo); return 0; }
    char linea[256];
    int n = 0;
    while (fgets(linea, sizeof linea, f)) {
        n++;
        char *c = strchr(linea, ';');
        if (c) *c = 0;
        char *s = trim(linea);
        /* etiquetas (pueden ir varias antes de la instrucción) */
        for (;;) {
            char *dp = strchr(s, ':');
            if (!dp) break;
            *dp = 0;
            char *nom = trim(s);
            if (netq < MAX_LABEL) {
                strncpy(etqs[netq].nombre, nom, sizeof etqs[netq].nombre - 1);
                etqs[netq].pc = nprog;
                netq++;
            }
            s = trim(dp + 1);
        }
        if (!*s) continue;
        char mn[16] = {0};
        int i = 0;
        while (s[i] && !isspace((unsigned char)s[i]) && i < 15) { mn[i] = s[i]; i++; }
        char *resto = trim(s + i);
        Op op;
        if (!op_desde_texto(mn, &op)) {
            fprintf(stderr, "Línea %d: instrucción desconocida '%s'\n", n, mn);
            fclose(f);
            return 0;
        }
        if (nprog >= MAX_INSTR) { fprintf(stderr, "Programa demasiado largo\n"); fclose(f); return 0; }
        Instr *in = &prog[nprog++];
        memset(in, 0, sizeof *in);
        in->op = op; in->linea = n;
        if (op == OP_JZ || op == OP_JNZ || op == OP_JMP) {
            strncpy(in->etq, resto, sizeof in->etq - 1);
        } else if (*resto) {
            char *coma = strchr(resto, ',');
            if (coma) { *coma = 0; in->b = parse_op(coma + 1); }
            in->a = parse_op(resto);
        }
    }
    fclose(f);
    return 1;
}

static int buscar_etq(const char *n, int linea) {
    for (int i = 0; i < netq; i++)
        if (strcmp(etqs[i].nombre, n) == 0) return etqs[i].pc;
    fprintf(stderr, "Línea %d: etiqueta '%s' no existe\n", linea, n);
    exit(1);
}

/* ---------- CPU ---------- */
static uint8_t val(Operando o) { return o.k == K_REG ? reg[o.v & 15] : (uint8_t)o.v; }
static uint8_t dir(Operando o) { return o.k == K_MEM_REG ? reg[o.v & 15] : (uint8_t)o.v; }

static uint8_t leer_puerto(uint8_t p) {
    if (p == 0x01) return p_estado;
    if (p == 0x02) { p_estado = 0; return p_dato; }   /* el hardware limpia el estado */
    return 0;
}

static void pulsar(uint8_t c) { p_dato = c; p_estado = 1; }

static int paso(void) {
    if (pc >= nprog) return 0;
    Instr *i = &prog[pc++];
    uint8_t r;
    switch (i->op) {
    case OP_MOV:   reg[i->a.v & 15] = val(i->b); break;
    case OP_LOAD:  reg[i->a.v & 15] = mem[dir(i->b)]; break;
    case OP_STORE: mem[dir(i->b)] = reg[i->a.v & 15]; break;
    case OP_IN:    reg[i->a.v & 15] = leer_puerto(dir(i->b)); break;
    case OP_OUT:   break;
    case OP_ADD:   r = reg[i->a.v & 15] + val(i->b); reg[i->a.v & 15] = r; zf = (r == 0); break;
    case OP_SUB:   r = reg[i->a.v & 15] - val(i->b); reg[i->a.v & 15] = r; zf = (r == 0); break;
    case OP_AND:   r = reg[i->a.v & 15] & val(i->b); reg[i->a.v & 15] = r; zf = (r == 0); break;
    case OP_CMP:   zf = ((uint8_t)(reg[i->a.v & 15] - val(i->b)) == 0); break;
    case OP_JZ:    if (zf)  pc = buscar_etq(i->etq, i->linea); break;
    case OP_JNZ:   if (!zf) pc = buscar_etq(i->etq, i->linea); break;
    case OP_JMP:   pc = buscar_etq(i->etq, i->linea); break;
    case OP_NOP:   break;
    case OP_HLT:   return 0;
    }
    return 1;
}

/* ---------- visualización ---------- */
static void texto(char *out, size_t n) {
    size_t k = 0;
    k += snprintf(out + k, n - k, "┌────┐\n");
    for (int f = 0; f < 4; f++) {
        k += snprintf(out + k, n - k, "│");
        for (int c = 0; c < 4; c++) {
            uint8_t v = mem[0xF0 + f * 4 + c];
            out[k++] = (v >= 32 && v < 127) ? (char)v : '.';
        }
        k += snprintf(out + k, n - k, "│\n");
    }
    k += snprintf(out + k, n - k, "└────┘\nCola 0x70..0x77:");
    for (int i = 0; i < 8; i++) k += snprintf(out + k, n - k, " %02X", mem[0x70 + i]);
    snprintf(out + k, n - k, "\nHEAD=%d  TAIL=%d  COUNT=%d  CURSOR=0x%02X\n",
             mem[0x78], mem[0x79], mem[0x7A], mem[0x60]);
}

/* ---------- modos de ejecución ---------- */
static void demo(const char *teclas) {
    char buf[512];
    for (const char *p = teclas; *p; p++) {
        uint8_t c = (uint8_t)*p;
        if (*p == '\\' && p[1]) {
            p++;
            c = (*p == 'b') ? 0x08 : (*p == 'n') ? '\n' : (*p == 't') ? '\t' : (uint8_t)*p;
        }
        pulsar(c);
        for (int i = 0; i < 400; i++) paso();
        if (c == 0x08) printf("--- tecla BACKSPACE ---\n");
        else           printf("--- tecla '%c' ---\n", c);
        texto(buf, sizeof buf);
        printf("%s\n", buf);
    }
}

static struct termios term_orig;
static void restaurar(void) { tcsetattr(STDIN_FILENO, TCSANOW, &term_orig); }
static void al_salir(int s) { (void)s; restaurar(); printf("\n"); _exit(0); }

static void interactivo(void) {
    tcgetattr(STDIN_FILENO, &term_orig);
    atexit(restaurar);
    signal(SIGINT, al_salir);
    struct termios t = term_orig;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0; t.c_cc[VTIME] = 0;      /* lectura no bloqueante */
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    char ult[512] = "", ahora[512];
    uint8_t pend[64]; int np = 0;
    printf("\x1b[2J");
    for (;;) {
        for (int i = 0; i < 300; i++) paso();
        uint8_t ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == 0x1b) break;
            if (ch == 0x7f) ch = 0x08;
            if (np < 64) pend[np++] = ch;
        }
        if (np > 0 && p_estado == 0) {
            pulsar(pend[0]);
            memmove(pend, pend + 1, --np);
        }
        texto(ahora, sizeof ahora);
        if (strcmp(ahora, ult) != 0) {
            printf("\x1b[H%s\nEscribe. Backspace borra. Esc para salir.\n", ahora);
            fflush(stdout);
            strcpy(ult, ahora);
        }
        usleep(5000);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s archivo.asm [\"teclas\\b...\"]\n", argv[0]);
        return 1;
    }
    if (!cargar(argv[1])) return 1;
    if (argc >= 3) demo(argv[2]);
    else           interactivo();
    return 0;
}
