#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>

using namespace std;

void inicializarMemoriaSecundaria() {
    ofstream archivo("memoria_secundaria.txt");
    for (int i = 1; i <= 1000; i++) {
        archivo << i * 10 << " ";
    }
    archivo.close();
}

int main() {
    inicializarMemoriaSecundaria();

    int arreglo1[1000];
    int arreglo2[100];

    ifstream archivo("memoria_secundaria.txt");
    if (!archivo.is_open()) {
        cerr << "Error al acceder a la memoria secundaria." << endl;
        return 1;
    }

    for (int i = 0; i < 1000; i++) {
        archivo >> arreglo1[i];
    }
    archivo.close();

    cout << "=== ARREGLO 1 (ESTADO INICIAL) ===" << endl;
    cout << "Primeros 5 elementos: ";
    for (int i = 0; i < 5; i++) {
        cout << arreglo1[i] << " ";
    }
    cout << "\nUltimos 5 elementos: ";
    for (int i = 995; i < 1000; i++) {
        cout << arreglo1[i] << " ";
    }
    cout << "\n\n";

    std::memcpy(arreglo2, &arreglo1[500], 100 * sizeof(int));
    std::memset(&arreglo1[500], 0, 100 * sizeof(int));

    cout << "=== ARREGLO 2 (ELEMENTOS TRANSFERIDOS DESDE INDICES 500-599) ===" << endl;
    for (int i = 0; i < 100; i++) {
        cout << setw(5) << arreglo2[i] << ((i + 1) % 10 == 0 ? "\n" : " ");
    }
    cout << "\n";

    cout << "=== VALIDACION DE LIMPIEZA EN ARREGLO 1 ===" << endl;
    bool cerosVerificados = true;
    for (int i = 500; i < 600; i++) {
        if (arreglo1[i] != 0) {
            cerosVerificados = false;
            break;
        }
    }

    if (cerosVerificados) {
        cout << "Validacion exitosa: Los indices 500 al 599 del arreglo original contienen exclusivamente ceros." << endl;
    } else {
        cout << "Error: Algunos elementos en el rango indicado no fueron reiniciados correctamente." << endl;
    }

    cout << "Muestra de verificacion (Indices 500 a 504 de Arreglo 1): ";
    for (int i = 500; i < 505; i++) {
        cout << arreglo1[i] << " ";
    }
    cout << endl;

    return 0;
}
