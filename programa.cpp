#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib> 
#include <algorithm> 
#include <chrono> 
#include <unordered_map>


using namespace std;
using namespace chrono; 


int charToIndex(char c) {
    switch (c) {
        case 'A': return 0;
        case 'G': return 1;
        case 'C': return 2;
        case 'T': return 3;
        default: return -1; // En caso de caracteres inválidos
    }
}

// Validar que las cadenas solo contengan caracteres válidos
void validarCadena(const string& cadena) {
    for (char c : cadena) {
        if (charToIndex(c) == -1) {
            cerr << "Error: La cadena contiene un carácter inválido: " << c << endl;
            exit(1);
        }
    }
}

// Función para leer la matriz de similitud desde un archivo
vector<vector<int>> leerMatrizDesdeArchivo(const string& nombreArchivo) {
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cerr << "Error: No se pudo abrir el archivo " << nombreArchivo << endl;
        exit(1);
    }

    vector<vector<int>> matriz(4, vector<int>(4)); // Matriz 4x4 para A, G, C, T
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!(archivo >> matriz[i][j])) {
                cerr << "Error: Formato incorrecto en el archivo de la matriz." << endl;
                exit(1);
            }
        }
    }

    archivo.close();
    return matriz;
}

// Generar archivo Graphviz para la matriz dp
void generarMatrizGraphviz(const vector<vector<int>>& dp, const vector<pair<int, int>>& rutaOptima,
                            const string& nombreArchivo, const string& alineamiento1, const string& alineamiento2) {
    ofstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cerr << "Error No se pudo crear el archivo Graphviz :( " << nombreArchivo << endl;
        return;
    }

    archivo << "digraph MatrizDP {\n";
    archivo << "node [shape=plaintext];\n";

    // Dibujar la matriz en formato tabla
    archivo << "tabla [label=<\n<table border='1' cellborder='1' cellspacing='0'>\n";

    // Agregar encabezado con la secuencia alineada de la cadena 2 (Columnas)
    archivo << "<tr><td></td>"; // Esquina superior izquierda vacía
    for (int j = 0; j < alineamiento2.size(); ++j) {
        archivo << "<td>" << alineamiento2[j] << "</td>";
    }
    archivo << "</tr>\n";

    // Agregar filas con la secuencia alineada de la cadena 1 (Filas)
    for (int i = 0; i < alineamiento1.size(); ++i) {
        archivo << "<tr><td>" << alineamiento1[i] << "</td>";  // Primer columna con la cadena 1
        for (int j = 0; j < alineamiento2.size(); ++j) {
            string style = " bgcolor=\"white\"";
            if (find(rutaOptima.begin(), rutaOptima.end(), make_pair(i, j)) != rutaOptima.end()) {
                style = " bgcolor=\"pink\""; // Resaltar la ruta óptima
            }
            archivo << "<td" << style << ">" << dp[i][j] << "</td>";  // Puntaje en cada celda
        }
        archivo << "</tr>\n";
    }

    archivo << "</table>>];\n";
    archivo << "}\n";
    archivo.close();
}

// Generar archivo Graphviz para el alineamiento de forma horizontal
void generarAlineamientoGraphviz(const string& alineamiento1, const string& alineamiento2, const string& nombreArchivo) {
    ofstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        cerr << "Error: No se pudo crear el archivo Graphviz " << nombreArchivo << endl;
        return;
    }

    archivo << "digraph Alineamiento {\n";
    archivo << "rankdir=TB;\n"; // Dirección de las cadenas horizontal
    archivo << "node [shape=plaintext];\n";

    archivo << "subgraph cadena1 {\n";
    archivo << "rank = same;\n";
    for (int i = 0; i < alineamiento1.size(); ++i) {
        archivo << "n1_" << i << " [label=\"" << alineamiento1[i] << "\"];\n";
    }
    archivo << "}\n";

    archivo << "subgraph cadena2 {\n";
    archivo << "rank = same;\n";
    for (int i = 0; i < alineamiento2.size(); ++i) {
        archivo << "n2_" << i << " [label=\"" << alineamiento2[i] << "\"];\n";
    }
    archivo << "}\n";

    for (int i = 0; i < alineamiento1.size(); ++i) {
        archivo << "n1_" << i << " -> n2_" << i << " [dir=none];\n";
    }

    archivo << "}\n";
    archivo.close();
}

// Implementación del algoritmo Needleman-Wunsch
void needlemanWunsch(const string& cadena1, const string& cadena2, const vector<vector<int>>& matrizU, int penalidadGap) {
    int n = cadena1.size();
    int m = cadena2.size();

    // Medir el tiempo de inicio
    auto inicio = high_resolution_clock::now();

    // Matriz dinámica para los puntajes
    vector<vector<int>> dp(n + 1, vector<int>(m + 1, 0));

    // Inicializar la primera fila y columna con penalidades
    for (int i = 1; i <= n; ++i) dp[i][0] = i * penalidadGap;
    for (int j = 1; j <= m; ++j) dp[0][j] = j * penalidadGap;

    // Llenar la matriz dp con la fórmula de recurrencia
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            int match = dp[i - 1][j - 1] + matrizU[charToIndex(cadena1[i - 1])][charToIndex(cadena2[j - 1])];
            int delete_ = dp[i - 1][j] + penalidadGap;
            int insert = dp[i][j - 1] + penalidadGap;
            dp[i][j] = max({match, delete_, insert});
        }
    }

    // Reconstrucción del alineamiento
    string alineamiento1 = "", alineamiento2 = "";
    vector<pair<int, int>> rutaOptima; // Guardar la ruta óptima
    int i = n, j = m;

    while (i > 0 || j > 0) {
        rutaOptima.push_back({i, j});
        if (i > 0 && j > 0 && dp[i][j] == dp[i - 1][j - 1] + matrizU[charToIndex(cadena1[i - 1])][charToIndex(cadena2[j - 1])]) {
            alineamiento1 = cadena1[i - 1] + alineamiento1;
            alineamiento2 = cadena2[j - 1] + alineamiento2;
            --i; --j;
        } else if (i > 0 && dp[i][j] == dp[i - 1][j] + penalidadGap) {
            alineamiento1 = cadena1[i - 1] + alineamiento1;
            alineamiento2 = "-" + alineamiento2;
            --i;
        } else {
            alineamiento1 = "-" + alineamiento1;
            alineamiento2 = cadena2[j - 1] + alineamiento2;
            --j;
        }
    }
    rutaOptima.push_back({0, 0});
    reverse(rutaOptima.begin(), rutaOptima.end());

    // Medir el tiempo de finalización
    auto fin = high_resolution_clock::now();
    auto duracion = duration_cast<nanoseconds>(fin - inicio);

    // Generar visualizaciones
    generarMatrizGraphviz(dp, rutaOptima, "matriz_dp.dot", alineamiento1, alineamiento2);
    generarAlineamientoGraphviz(alineamiento1, alineamiento2, "alineamiento.dot");

    // Resultado
    cout << "Puntaje máximo: " << dp[n][m] << endl;
    cout << "Alineamiento 1: " << alineamiento1 << endl;
    cout << "Alineamiento 2: " << alineamiento2 << endl;
    cout << "Tiempo de ejecución: " << duracion.count() << " nanosegundos" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 9) {
        cerr << "Uso: " << argv[0] << " -C1 archivo1 -C2 archivo2 -U archivoMatriz -V penalidad" << endl;
        return 1;
    }

    // Leer argumentos
    string archivo1 = argv[2];
    string archivo2 = argv[4];
    string archivoMatriz = argv[6];
    int penalidadGap = atoi(argv[8]);

    // Leer cadenas de los archivos
    ifstream input1(archivo1), input2(archivo2);
    if (!input1.is_open() || !input2.is_open()) {
        cerr << "Error, no se pudieron abrir los archivos de entrada " << endl;
        return 1;
    }
    string cadena1, cadena2;
    input1 >> cadena1;
    input2 >> cadena2;

    input1.close();
    input2.close();

    // Validar cadenas
    validarCadena(cadena1);
    validarCadena(cadena2);

    // Leer matriz de similitud
    vector<vector<int>> matrizU = leerMatrizDesdeArchivo(archivoMatriz);

    // Ejecutar el algoritmo
    needlemanWunsch(cadena1, cadena2, matrizU, penalidadGap);

    return 0;
}