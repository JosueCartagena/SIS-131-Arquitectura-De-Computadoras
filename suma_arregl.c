#include <stdio.h>

int main() {
    int arreglo[] = {5, 12,5,2,1,};
    int tamano = sizeof(arreglo) / sizeof(arreglo[0]);
    int suma = 0;
    for (int i = 0; i < tamano; i++) {
        suma += arreglo[i];
    }

    printf("La suma total es: %d\n", suma);
    return 0;
}
