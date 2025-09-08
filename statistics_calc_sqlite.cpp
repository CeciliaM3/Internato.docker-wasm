extern "C" {
    #include "sqlite3.h"
}
#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <chrono>
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::array;
using std::vector;
using std::string;
using std::to_string;
using std::replace;
using std::find;
using std::stoi;
using std::stod;
using std::isnan;
using std::numeric_limits;
using std::min_element;
using std::max_element;
using std::accumulate;
using std::abs;
using std::transform;
using std::getenv;
using namespace std::chrono;

struct CallbackContext {
    vector<array<double, 6>> extracted_data;
    vector<double> oilactivation_column;
};

int callback(void* output, int argc, char** argv, char** colNames);

int main(int argc, char* argv[]) {

    // inizio della misurazione del tempo impiegato per l'esecuzione di tutto il programma
    auto start_time = high_resolution_clock::now();

    if (argc != 2) {
        cerr << "Use: " << argv[0] << " <number of tuples to consider>" << endl;
        return 1;
    }

    int num_tuples = stoi(argv[1]);
    if (num_tuples < 1) {
        cerr << "Error: number of tuples must be a positive integer." << endl;
        return 1;
    }
    if (num_tuples > 1516949) {
        num_tuples = 1516949;
    }

    sqlite3 *db; 
    const char* db_name = "db/aircompressor.db";

    // Apertura della connessione al database SQLite aircompressor.db
    int err = sqlite3_open(db_name, &db); 
    if (err != SQLITE_OK) {
        cerr << "Attempt to establish connection to database failed: " << sqlite3_errmsg(db) << endl;
        return 1;
    }

    // DEFINIZIONE DELLA QUERY PER PRELEVARE IL NUMERO RICHIESTO DI TUPLE
    string sql_query = "SELECT TP2, TP3, H1, DV_pressure, Reservoirs, Oil_temperature, Oil_level FROM aircompressor_data LIMIT " + to_string(num_tuples);

    // dichiarazione dell'oggetto contenente i buffer da passare alla callback per accumulare progressivamente i risultati della query 
    CallbackContext ctx;

    // ESECUZIONE DELLA QUERY tramite query diretta a database sqlite
    err = sqlite3_exec(db, sql_query.c_str(), callback, &ctx, nullptr);
    if (err != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    // Chiusura delle risorse
    sqlite3_close(db);

    // EFFETTUAZIONE DELLE COMPUTAZIONI E STAMPA SU STDOUT DEI RISULTATI
    // 1) Differenza media, massima e minima tra Reservoirs e TP3 (dato che dovrebbero sempre avere valore simile)
    vector<double> reservoisTP3diffs;
    reservoisTP3diffs.reserve(ctx.extracted_data.size());

    for (const auto& data_row : ctx.extracted_data) {
        reservoisTP3diffs.push_back(data_row[4] - data_row[1]);
    }
    array<double, 3> reservoisTP3diffs_stats;
    reservoisTP3diffs_stats[0] = accumulate(reservoisTP3diffs.begin(), reservoisTP3diffs.end(), 0.0)/reservoisTP3diffs.size();
    reservoisTP3diffs_stats[1] = abs(*(max_element(reservoisTP3diffs.begin(), reservoisTP3diffs.end())));
    reservoisTP3diffs_stats[2] = abs(*(min_element(reservoisTP3diffs.begin(), reservoisTP3diffs.end())));

    cout << "### DIFFERENCES RESERVOIRS - TP3 (avg, max, min) ###" << endl;
    for (const auto& value : reservoisTP3diffs_stats) {
            cout << value << "\t";
        }

    // 2) Percentuale di volte il cui il segnale elettrico Oil_level, che indica che il livello dell'olio è inferiore ai valori attesi, è attivo
    double oilactivation_percentage;

    int activation_occurrences = count_if(ctx.oilactivation_column.begin(), ctx.oilactivation_column.end(), 
        [](double value) {
            return value == 1.0;
        });

    oilactivation_percentage = (static_cast<double>(activation_occurrences) / ctx.extracted_data.size()) * 100;

    cout << endl << "\n### OIL LEVEL ACTIVATION PERCENTAGE ###" << endl;
    cout << oilactivation_percentage << "%" << endl;

    // Calcolo delle medie
    array<double, 6> means;
    for (int i = 0; i < 6; ++i) {
        means[i] = accumulate(ctx.extracted_data.begin(), ctx.extracted_data.end(), 0.0, [i](double sum, const array<double, 6>& data_row) { return sum + data_row[i]; })/ctx.extracted_data.size();  
    }

    // 3. Calcolo dell'indice di correlazione di Pearson tra le variabili TP2 (indice [0]) e Oil_temperature (indice [5])
    double TP2_oiltemp_corr;
    double num = 0.0, den_1 = 0.0, den_2 = 0.0;
    if(!isnan(means[0]) && !isnan(means[5])) {
        int num_notnan = 0;
        for (int j = 0; j < ctx.extracted_data.size(); j++) {
            if(!isnan(ctx.extracted_data[j][0]) && !isnan(ctx.extracted_data[j][5])) {
                num += (ctx.extracted_data[j][0] - means[0])*(ctx.extracted_data[j][5] - means[5]);
                den_1 += pow((ctx.extracted_data[j][0] - means[0]), 2);
                den_2 += pow((ctx.extracted_data[j][5] - means[5]), 2);
                num_notnan = 1;
            }
        }
        if (num_notnan && den_1 > 0 && den_2 > 0) {  
            TP2_oiltemp_corr = num / sqrt(den_1 * den_2);
        } else {
            TP2_oiltemp_corr = numeric_limits<double>::quiet_NaN();  
        }
    } else {
        TP2_oiltemp_corr = numeric_limits<double>::quiet_NaN();
    }
    cout << "\n### CORRELATION BETWEEN TP2 AND OIL TEMPERATURE ###" << endl << TP2_oiltemp_corr << endl;

    // Calcolo di varianze e deviazioni standard
    array<double, 6> variances;
    for(int i = 0; i < means.size(); i++) {
        if(!isnan(means[i])) {
            double accumulator = 0.0; 
            int nans = 0;
            for (int j = 0; j < ctx.extracted_data.size(); j++) {
                if (!isnan(ctx.extracted_data[j][i])) {
                    accumulator += pow((ctx.extracted_data[j][i] - means[i]), 2);
                } else {
                    nans++;
                }
            }
            int denominator = ctx.extracted_data.size() - nans;
            variances[i] = (denominator > 0) ? (accumulator / denominator) : numeric_limits<double>::quiet_NaN();
        } else {
            variances[i] = numeric_limits<double>::quiet_NaN();
        }
    }

    array<string, 6> column_names; 
    column_names[0] = "TP2";
    column_names[1] = "TP3";
    column_names[2] = "H1";
    column_names[3] = "DV_pressure";
    column_names[4] = "Reservoirs";
    column_names[5] = "Oil_temperature";
    
    cout << "\n### AVERAGE, VARIANCE, STANDARD DEVIATION ###" << endl;
    for(int i = 0; i < variances.size(); i++) {
        cout << column_names[i] << " " << means[i] << " " << variances[i] << " " << sqrt(variances[i]) << endl;
    }
    cout << endl;

    // Identificazione del numero di anomalie per ogni campo considerato
    array<double, 6> thrice_stddevs;
    transform(variances.begin(), variances.end(), thrice_stddevs.begin(), [](double var) {
        return 3 * sqrt(var);
    });
    
    cout << "### Number of anomalies (values outside of the avg +/- 3*stddev interval) per column ###" << endl;
    
    int anomalies_counter = 0; 
    for(int i = 0; i < thrice_stddevs.size(); i++) {
        cout << column_names[i] << " ";
        anomalies_counter = 0;
        if(!isnan(means[i]) && !isnan(thrice_stddevs[i])) {
            for (int j = 0; j < ctx.extracted_data.size(); j++) {
                if(!isnan(ctx.extracted_data[j][i]) && (ctx.extracted_data[j][i] < means[i] - thrice_stddevs[i] || ctx.extracted_data[j][i] > means[i] + thrice_stddevs[i])) {
                    anomalies_counter++;
                }
            }
        } else {
            cout << "NaN";
        }
        cout << anomalies_counter << endl;
    }

    cout << endl << "Calculations performed considering " << num_tuples << " tuples." << endl;

    // termine della misurazione del tempo impiegato per l'esecuzione di tutto il programma e calcolo del risultato
    auto end_time = high_resolution_clock::now();
    duration<double, std::milli> execution_duration = end_time - start_time;

    cout << endl << "Execution time for the full C++ program (measured from within): " << execution_duration.count() << " milliseconds" << endl;

    return 0;
}

// Funzione callback per scrivere i risultati della query su un file
int callback(void* ctxarg, int argc, char** argv, char** colNames) {
    CallbackContext * ctx = static_cast<CallbackContext*>(ctxarg);

    array<double, 6> extracted_row;
    for (int i = 0; i < argc - 1; i++) {
        extracted_row[i] = argv[i] ? stod(argv[i]) : numeric_limits<double>::quiet_NaN();
    }
    ctx->extracted_data.push_back(extracted_row);
    ctx->oilactivation_column.push_back(argv[argc - 1] ? stod(argv[argc - 1]) : numeric_limits<double>::quiet_NaN());

    return 0; 
}
