#include <iostream>
#include <cstring> // Requerido para memcpy y memset

using namespace std;

int main() {
    int arreglo1[1000];
    int arreglo2[100];

    // 1. Simulación de lectura e inicialización de 1000 elementos en memoria
    for (int i = 0; i < 1000; i++) {
        arreglo1[i] = i + 1;
    }

    // 3. Salida por consola: primeros y últimos 5 elementos del arreglo inicial
    cout << "--- PRIMEROS 5 ELEMENTOS DEL ARREGLO INICIAL ---" << endl;
    for (int i = 0; i < 5; i++) {
        cout << arreglo1[i] << " ";
    }
    cout << "\n\n--- ULTIMOS 5 ELEMENTOS DEL ARREGLO INICIAL ---" << endl;
    for (int i = 995; i < 1000; i++) {
        cout << arreglo1[i] << " ";
    }
    cout << "\n" << endl;

    // 2. Transferencia de los índices 500 a 599 (100 elementos) a arreglo2 usando memcpy
    memcpy(arreglo2, &arreglo1[500], 100 * sizeof(int));

    // Limpieza de los valores transferidos en el arreglo original (estableciéndolos a cero)
    memset(&arreglo1[500], 0, 100 * sizeof(int));

    // Salida por consola: 100 elementos transferidos al segundo arreglo
    cout << "--- 100 ELEMENTOS TRANSFERIDOS AL SEGUNDO ARREGLO ---" << endl;
    for (int i = 0; i < 100; i++) {
        cout << arreglo2[i] << " ";
    }
    cout << "\n" << endl;

    // Validación de que los índices 500 al 599 contienen ceros tras la operación
    bool es_valido = true;
    for (int i = 500; i <= 599; i++) {
        if (arreglo1[i] != 0) {
            es_valido = false;
            break;
        }
    }

    cout << "--- VALIDACION DE MEMORIA ---" << endl;
    if (es_valido) {
        cout << "EXITO: Los indices 500 al 599 del primer arreglo contienen 0 correctamente." << endl;
    } else {
        cout << "ERROR: La limpieza de memoria fallo." << endl;
    }

    return 0;
}
