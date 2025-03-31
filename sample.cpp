#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sqlite3.h>
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::array;
using std::vector;
using std::string;
using std::to_string;
using std::stoi;
using std::stod;
using std::nan;
using std::isnan;
using std::transform;

int callback_tofile_andcout(void* output, int argc, char** argv, char** colNames);
int callback_fordata(void* given_struct, int argc, char** argv, char** colNames);
int callback_formeans(void* given_struct, int argc, char** argv, char** colNames);

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
    
    sqlite3 *db; 
    const char* db_name = "data/aircompressor_database.db";

    // Apertura della connessione al database SQLite aircompressor_database.db
    int err = sqlite3_open(db_name, &db); 
    if (err != SQLITE_OK) {
        cerr << "Impossibile stabilire una connessione al database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }

    // Apertura del file di output su cui scrivere i risultati delle query e delle computazioni
    const char *filename = "data/statistics_output.txt";
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cerr << "Impossibile aprire il file di output." << endl;
        sqlite3_close(db);
        return 1;
    }

    /*
    Obiettivi: calcolo di:
    1) Differenza media, massima e minima tra Reservoirs e TP3 (dato che dovrebbero sempre avere valore simile)
    2) Percentuale di volte il cui il segnale elettrico Oil_level, che indica che il livello dell'olio è inferiore ai valori attesi, è attivo
    3) Correlazione di Pearson tra TP2 e Oil_temperature
    4) media, varianza e deviazione standard per TP2, TP3, H1, CV_pressure, Reservoirs e Oil_temperature
    5) Identificazione ed elenco delle anomalie (valori al di fuori del ranga media +/- 3*stddev) per TP2, TP3, H1, CV_pressure, Reservoirs e Oil_temperature
    */

    // DEFINIZIONI DELLE QUERIES
    // Stringa per la limitazione delle tuple considerate al valore richiesto e query di base per ridimensionare di conseguenza il set di dati su cui opereranno le altre query
    string limit_clause = " LIMIT " + to_string(num_tuples);
    string sql_data_limited = "SELECT timestamp, TP2, TP3, H1, DV_pressure, Reservoirs, Oil_temperature, Oil_level FROM data" + limit_clause;

    // 1. 
    string sql_reservois_TP3_diff = 
    "SELECT AVG(Reservoirs - TP3), "
    "MAX(Reservoirs - TP3), "
    "MIN(Reservoirs - TP3) "
    "FROM(" + sql_data_limited + ");";

    // 2. 
    string sql_oillevel_activation = 
    "SELECT 100.0 * ("
        "(SELECT COUNT(Oil_level) FROM (" + sql_data_limited + ") WHERE Oil_level = 1.0) * 1.0/"
        "(SELECT COUNT(Oil_level) FROM (" + sql_data_limited + ")));";

    // 3/4/5.
    string sql_data = "SELECT TP2, TP3, H1, DV_pressure, Reservoirs, Oil_temperature FROM data" + limit_clause + ";";
    string sql_means = "SELECT AVG(TP2), AVG(TP3), AVG(H1), AVG(DV_pressure), AVG(Reservoirs), AVG(Oil_temperature) FROM (" + sql_data_limited + ");";

    // ESECUZIONE DELLE QUERIES
    cout << "### DIFFERENZE RESERVOIRS - TP3 (avg, max, min) ###" << endl;
    outFile << "### DIFFERENZE RESERVOIRS - TP3 (avg, max, min) ###" << endl;
    char* errorstr = nullptr;
    if (sqlite3_exec(db, sql_reservois_TP3_diff.c_str(), callback_tofile_andcout, &outFile, &errorstr) != SQLITE_OK) {
        cerr << "Errore SQL nella query sql_reservois_TP3_diff : " << errorstr << endl;
        sqlite3_free(errorstr);  
    }

    cout << "\n### PERCENTUALE ATTIVAZIONE OIL_LEVEL ###" << endl;
    outFile << "\n### PERCENTUALE ATTIVAZIONE OIL_LEVEL ###" << endl;
    if (sqlite3_exec(db, sql_oillevel_activation.c_str(), callback_tofile_andcout, &outFile, &errorstr) != SQLITE_OK) {
        cerr << "Errore SQL nella query sql_oillevel_activation: " << errorstr << endl;
        sqlite3_free(errorstr);
    }

    vector<array<double, 6>> extracted_data;
    if (sqlite3_exec(db, sql_data.c_str(), callback_fordata, &extracted_data, &errorstr) != SQLITE_OK) {
        cerr << "Errore SQL nella query sql_data: " << errorstr << endl;
        sqlite3_free(errorstr);
    }

    array<double, 6> extracted_means;
    if (sqlite3_exec(db, sql_means.c_str(), callback_formeans, &extracted_means, &errorstr) != SQLITE_OK) {
        cerr << "Errore SQL nella query sql_means: " << errorstr << endl;
        sqlite3_free(errorstr);
    }

    // COMPUTAZIONI ULTERIORI
    //Calcolo dell'indice di correlazione di Pearson tra le variabili TP2 (indice [0]) e Oil_temperature (indice [5])
    double TP2_oiltemp_corr;
    double num = 0.0, den_1 = 0.0, den_2 = 0.0;
    if(!isnan(extracted_means[0]) && !isnan(extracted_means[5])) {
        int num_notnan = 0;
        for (int j = 0; j < extracted_data.size(); j++) {
            if(!isnan(extracted_data[j][0]) && !isnan(extracted_data[j][5])) {
                num += (extracted_data[j][0] - extracted_means[0])*(extracted_data[j][5] - extracted_means[5]);
                den_1 += pow((extracted_data[j][0] - extracted_means[0]), 2);
                den_2 += pow((extracted_data[j][5] - extracted_means[5]), 2);
                num_notnan = 1;
            }
        }
        if (num_notnan && den_1 > 0 && den_2 > 0) {  
            TP2_oiltemp_corr = num / sqrt(den_1 * den_2);
        } else {
            TP2_oiltemp_corr = nan("");  
        }
    } else {
        TP2_oiltemp_corr = nan("");
    }
    cout << "\n### CORRELAZIONE TRA TP2 E OIL TEMPERATURE ###" << endl << TP2_oiltemp_corr << endl;
    outFile << "\n### CORRELAZIONE TRA TP2 E OIL TEMPERATURE ###" << endl << TP2_oiltemp_corr << endl;

    // Calcolo di varianze e deviazioni standard
    array<double, 6> variances;
    for(int i = 0; i < extracted_means.size(); i++) {
        if(!isnan(extracted_means[i])) {
            double accumulator = 0.0; 
            int nans = 0;
            for (int j = 0; j < extracted_data.size(); j++) {
                if (!isnan(extracted_data[j][i])) {
                    accumulator += pow((extracted_data[j][i] - extracted_means[i]), 2);
                } else {
                    nans++;
                }
            }
            int denominator = extracted_data.size() - nans;
            variances[i] = (denominator > 0) ? (accumulator / denominator) : nan("");
        } else {
            variances[i] = nan("");
        }
    }

    array<string, 6> column_names; 
    column_names[0] = "TP2";
    column_names[1] = "TP3";
    column_names[2] = "H1";
    column_names[3] = "DV_pressure";
    column_names[4] = "Reservoirs";
    column_names[5] = "Oil_temperature";
    cout << "\n### MEDIA, VARIANZA, DEVIAZIONE STANDARD ###" << endl;
    outFile << "\n### MEDIA, VARIANZA, DEVIAZIONE STANDARD ###" << endl;
    for(int i = 0; i < variances.size(); i++) {
        cout << column_names[i] << ": " << extracted_means[i] << ", " << variances[i] << ", " << sqrt(variances[i]) << endl;
        outFile << column_names[i] << ": " << extracted_means[i] << ", " << variances[i] << ", " << sqrt(variances[i]) << endl;
    }
    cout << endl;
    outFile << endl;

    array<double, 6> thrice_stddevs;
    transform(variances.begin(), variances.end(), thrice_stddevs.begin(), [](double var) {
        return 3 * sqrt(var);
    });

    // Identificazione delle anomalie
    outFile << "### ANOMALIE (valori al di fuori dell'intervallo media +/- 3*stddev) ###" << endl;
    for(int i = 0; i < thrice_stddevs.size(); i++) {
        outFile << column_names[i] << endl;
        if(!isnan(extracted_means[i]) && !isnan(thrice_stddevs[i])) {
            for (int j = 0; j < extracted_data.size(); j++) {
                if(!isnan(extracted_data[j][i]) && (extracted_data[j][i] < extracted_means[i] - thrice_stddevs[i] || extracted_data[j][i] > extracted_means[i] + thrice_stddevs[i])) {
                    outFile << extracted_data[j][i] << endl;
                }
            }
        } else {
            outFile << "NaN";
        }
        outFile << endl;
    }

    // Chiusura delle risorse e messaggio di conferma
    cout << "Computazioni effettuate con successo su " << num_tuples << " tuple (dattagli anomalie in " << filename << ")." << endl;
    sqlite3_close(db);
    outFile.close();

    return 0;
}

// Funzione callback per gestire i dati restituiti dalle query, nei casi essi debbano essere stampati direttamente sul file di output (obiettivi 1, 2)
int callback_tofile_andcout(void* output, int argc, char** argv, char** colNames) {
    if (!output) {
        cerr << "File di output non trovato." << endl;
        return 1;  
    }

    ofstream* outFile = static_cast<ofstream*>(output);

    if (!outFile->is_open()) {
        cerr << "Impossibile aprire il file di output (dall'interno della callback)." << endl;
        return 1;  
    }

    for (int i = 0; i < argc; i++) {
        *outFile << (argv[i] ? argv[i] : "NULL") << "\t";
        cout << (argv[i] ? argv[i] : "NULL") << "\t";
    }

    *outFile << endl;
    cout << endl;

    return 0; 
}

// Funzioni callback per gestire i dati restituiti dalle query, nei casi essi debbano essere ulteriormente elaborati dal programma (obiettivi 3, 4, 5)
int callback_fordata(void* given_struct, int argc, char** argv, char** colNames) {
    if (!given_struct) {
        cerr << "Struttura dati per la memorizzazione provvisoria dei dati estratti dal database non trovata." << endl;
        return 1;  
    }

    auto* extracted_data = static_cast<vector<array<double, 6>>*>(given_struct);

    array<double, 6> row;
    for (int i = 0; i < argc; i++) {
        row[i] = argv[i] ? stod(argv[i]) : nan("");  
    }

    extracted_data->push_back(row);

    return 0; 
}

int callback_formeans(void* given_struct, int argc, char** argv, char** colNames) {
    if (!given_struct) {
        cerr << "Struttura dati per la memorizzazione provvisoria delle medie estratte dal database non trovata." << endl;
        return 1;  
    }

    auto* extracted_means = static_cast<array<double, 6>*>(given_struct);

    for (int i = 0; i < argc; i++) {
        (*extracted_means)[i] = argv[i] ? stod(argv[i]) : nan("");  
    }

    return 0; 
}