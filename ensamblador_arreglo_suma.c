#include <stdio.h>

int main() {
    int numeros[4];
    int suma = 0;

    __asm__ (
        "movl $10, 0(%1)\n\t"
        "movl $20, 4(%1)\n\t"
        "movl $30, 8(%1)\n\t"
        "movl $40, 12(%1)\n\t"

        "movl 0(%1), %%eax\n\t"
        "addl 4(%1), %%eax\n\t"
        "addl 8(%1), %%eax\n\t"
        "addl 12(%1), %%eax\n\t"

        "movl %%eax, %0\n\t"
        : "=r" (suma)
        : "r" (numeros)
        : "%eax", "memory"
    );

    printf("Elementos del arreglo: %d, %d, %d, %d\n", numeros[0], numeros[1], numeros[2], numeros[3]);
    printf("Suma total: %d\n", suma);

    return 0;
}
