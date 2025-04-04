#include <iostream>
#include <fstream>
#include <filesystem>
//#include <sqlite3.h>
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stoi;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <numero di tuple da considerare>" << endl;
        return 1;
    }

    int num_tuples = stoi(argv[1]);
    /*
    if (num_tuples < 10) {
        cerr << "Errore: il numero di tuple deve essere almeno di 10" << endl;
        return 1;
    }
    */
    
    // Apertura del file di input (csv)
    const char *input_filename = "data/cleaned.csv";
    ifstream inFile(input_filename);
    if (!inFile.is_open()) {
        cerr << "Impossibile aprire il file di input." << endl;
        return 1;
    }

    // Apertura del file di output su cui scrivere i contenuti letti da quello di input
    const char *output_filename = "data/statistics_output.txt";
    ofstream outFile(output_filename);
    if (!outFile.is_open()) {
        cerr << "Impossibile aprire il file di output." << endl;
        inFile.close();
        return 1;
    }

    // lettura e riscrittura delle informazioni
    string line;
    int count = 0; 

    while (count < num_tuples && getline(inFile, line)) {
        outFile << line << endl;
        cout << line << endl; // Aggiunto per debug
        count++;
    }

    // Chiusura delle risorse e messaggio di conferma
    inFile.close();
    outFile.close();

    return 0;
}