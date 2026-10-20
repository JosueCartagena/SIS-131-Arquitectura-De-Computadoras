#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOQUE (100UL * 1024 * 1024)  /* 100 MB por iteración */

int main(void) {
    unsigned long total = 0;
    while (1) {
        char *p = malloc(BLOQUE);
        if (!p) { printf("malloc falló. Total: %lu MB\n", total / (1024*1024)); break; }
        memset(p, 1, BLOQUE);   /* toca las páginas para que se asignen realmente */
        total += BLOQUE;
        printf("Asignado: %lu MB\n", total / (1024*1024));
        fflush(stdout);
        sleep(1);
    }
    pause();  /* mantiene el proceso vivo para observar el swap */
    return 0;
}
