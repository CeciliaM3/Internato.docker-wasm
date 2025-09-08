#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <chrono>
#include <emscripten/fetch.h>
#include <emscripten.h>
#include <cstdio>
#include "json.hpp"
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
using std::abs;
using std::transform;
using std::atomic;
using nlohmann::json;
using namespace std::chrono;

struct RequestContext {
    int timeout_id;
    string *JSON_buffer;
};

int num_tuples;
atomic<int> completed_requests = 0; 
atomic<bool> timeout_triggered = false; 
string JSON_buffer;
emscripten_fetch_t* fetch_struct;
time_point<high_resolution_clock> start_time;

void callback_success(emscripten_fetch_t* fetch);
void callback_failure(emscripten_fetch_t* fetch);
extern "C" void callback_timeout(const char* payload);
void main_loop();

EM_JS(int, set_timeout, (int delay_ms, const char* payload, int func_ptr), {
    return setTimeout(function() {
        dynCall('vi', func_ptr, [payload]);
    }, delay_ms);
});

EM_JS(void, clear_timeout, (int id), {
    clearTimeout(id);
});

int main(int argc, char* argv[]) {

    // inizio della misurazione del tempo impiegato per l'esecuzione di tutto il programma
    start_time = high_resolution_clock::now();

    string host_addr = "localhost";

    switch(argc) {
        case 2: 
            num_tuples = stoi(argv[1]);
            break;
        case 3:
            num_tuples = stoi(argv[2]);
            host_addr = argv[1];
            break;
        default:
            cerr << "Use: " << argv[0] << " [<host_address>] <number of tuples to consider> " << endl;
            return 1;
    }

    if (num_tuples < 1) {
        cerr << "Error: number of tuples must be a positive integer." << endl;
        return 1;
    }
    if (num_tuples > 1516949) {
        num_tuples = 1516949;
    }

    // determinazione dell'indirizzo/nome a cui far riferimento per contattare la porta 8000 su cui è in ascolto il servizio fastAPI
    string url = "http://" + host_addr + ":8000/query";

    /*
    Obiettivi: calcolo di:
    1) Differenza media, massima e minima tra Reservoirs e TP3 (dato che dovrebbero sempre avere valore simile)
    2) Percentuale di volte il cui il segnale elettrico Oil_level, che indica che il livello dell'olio è inferiore ai valori attesi, è attivo
    3) Correlazione di Pearson tra TP2 e Oil_temperature
    4) media, varianza e deviazione standard per TP2, TP3, H1, CV_pressure, Reservoirs e Oil_temperature
    5) Identificazione ed elenco delle anomalie (valori al di fuori del ranga media +/- 3*stddev) per TP2, TP3, H1, CV_pressure, Reservoirs e Oil_temperature
    */

    // DEFINIZIONE DELLA QUERY PER PRELEVARE IL NUMERO RICHIESTO DI TUPLE
    string sql_query = "SELECT TP2, TP3, H1, DV_pressure, Reservoirs, Oil_temperature, Oil_level FROM aircompressor_data LIMIT " + to_string(num_tuples);

    // preparazione della variabile per trasformare le query in payload pronto per essere trasmesso come corpo di una richiesta http
    json body;
    body["query"] = sql_query;
    string payload = body.dump();

    // ESECUZIONE DELLA QUERY tramite richiesta http ad app.py con fastAPI, che si interfaccia poi con il database server MySQL
    // Inizializzazione delle strutture necessarie ad eseguire la richiesta http tramite l'API fetch di emscripten e impostazione delle configurazioni adeguate
    emscripten_fetch_attr_t fetch_attr;
    emscripten_fetch_attr_init(&fetch_attr);

    strcpy(fetch_attr.requestMethod, "POST");

    static const char* headers[] = {
        // "Content-Type: application/json",
        "Accept: application/json",
        nullptr
    };
    fetch_attr.requestHeaders = headers;
    fetch_attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    fetch_attr.onsuccess = callback_success;
    fetch_attr.onerror = callback_failure;

    // Chiamate asincrone
    RequestContext request_context;
    fetch_attr.requestData = payload.c_str();
    fetch_attr.requestDataSize = payload.size();
    
    request_context.JSON_buffer = &JSON_buffer;
    fetch_attr.userData = &request_context;

    fetch_struct = emscripten_fetch(&fetch_attr, url.c_str());
    
    if(!fetch_struct) {
        cerr << "emscripten_fetch() failed." << endl;
        return 1;
    }
    request_context.timeout_id = set_timeout(120000, payload.c_str(), (int)&callback_timeout);

    emscripten_set_main_loop(main_loop, 0, 1);
}

// funzione di callback per raccogliere i dati in formato JSON contenuti nella risposta inviata da fastAPI
void callback_success(emscripten_fetch_t *fetch) {
    if (timeout_triggered) {
        return;
    }

    RequestContext* ctx = static_cast<RequestContext*>(fetch->userData);
    clear_timeout(ctx->timeout_id);

    string* JSON_buffer = nullptr;
    if(ctx->JSON_buffer && fetch->data && fetch->numBytes > 0) {
        *(ctx->JSON_buffer) = string(fetch->data, fetch->numBytes);
    }
    
    completed_requests ++;
    emscripten_fetch_close(fetch);
}

// funzione chiamata in caso di fallimento della richiesta
void callback_failure(emscripten_fetch_t *fetch) {
    if (timeout_triggered) {
        return;
    }

    RequestContext* ctx = static_cast<RequestContext*>(fetch->userData);
    clear_timeout(ctx->timeout_id);

    cerr << "Request failed with status code: " << fetch->status << endl;

    completed_requests ++;
    emscripten_fetch_close(fetch);
}

// funzione di callback attivata allo scadere del timeout
extern "C" void callback_timeout(const char* payload) {
    if (timeout_triggered) {
        return;
    }

    cerr << "Timeout reached for the HTTP request. Aborting program." << endl;
    timeout_triggered = true;
    if (fetch_struct) {
        RequestContext* ctx = static_cast<RequestContext*>(fetch_struct->userData);
        clear_timeout(ctx->timeout_id);
        emscripten_fetch_close(fetch_struct);
    }
    emscripten_cancel_main_loop();  
    emscripten_force_exit(1);  
}

void main_loop() {
    if (timeout_triggered) {
        return;  
    }

    // sezione di computazioni da svolgere una volta ricevute tutte e 4 le risposte
    if (completed_requests >= 1) {
        // dichiarazione dei buffer per il salvataggio dei risultati delle query 
        // (sia come stringhe per accogliere inizialmente i dati in formato JSON, sia come buffer dei giusti tipi di dati per poterli usare nelle computazioni successive)
        vector<array<double, 6>> extracted_data;
        vector<double> oilactivation_column;
        
        // parsing delle risposte JSON
        json parsed_data;

        try {
            parsed_data = json::parse(JSON_buffer);
            for (const auto& row : parsed_data) {
                array<double, 6> parsed_row;
                parsed_row[0] = row["TP2"].is_null() ? numeric_limits<double>::quiet_NaN() : row["TP2"].get<double>();
                parsed_row[1] = row["TP3"].is_null() ? numeric_limits<double>::quiet_NaN() : row["TP3"].get<double>(); 
                parsed_row[2] = row["H1"].is_null() ? numeric_limits<double>::quiet_NaN() : row["H1"].get<double>(); 
                parsed_row[3] = row["DV_pressure"].is_null() ? numeric_limits<double>::quiet_NaN() : row["DV_pressure"].get<double>(); 
                parsed_row[4] = row["Reservoirs"].is_null() ? numeric_limits<double>::quiet_NaN() : row["Reservoirs"].get<double>(); 
                parsed_row[5] = row["Oil_temperature"].is_null() ? numeric_limits<double>::quiet_NaN() : row["Oil_temperature"].get<double>(); 
                extracted_data.push_back(parsed_row);
                oilactivation_column.push_back(row["Oil_level"].is_null() ? numeric_limits<double>::quiet_NaN() : row["Oil_level"].get<double>());
            }
        }
        catch (const json::parse_error& e) {
            cerr << "Parsing error: " << e.what() << endl;
        }
        catch (const json::type_error& e) {
            cerr << "Type error during parsing: " << e.what() << endl;
        }
        catch (const std::exception& e) {
            cerr << "Generic error during parsing: " << e.what() << endl;
        }

        // EFFETTUAZIONE DELLE COMPUTAZIONI E STAMPA SU STDOUT DEI RISULTATI
        // 1) Differenza media, massima e minima tra Reservoirs e TP3 (dato che dovrebbero sempre avere valore simile)
        vector<double> reservoisTP3diffs;
        reservoisTP3diffs.reserve(extracted_data.size());

        for (const auto& data_row : extracted_data) {
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

        int activation_occurrences = count_if(oilactivation_column.begin(), oilactivation_column.end(), 
            [](double value) {
                return value == 1.0;
            });

        oilactivation_percentage = (static_cast<double>(activation_occurrences) / extracted_data.size()) * 100;

        cout << endl << "\n### OIL LEVEL ACTIVATION PERCENTAGE ###" << endl;
        cout << oilactivation_percentage << "%" << endl;

        // Calcolo delle medie
        array<double, 6> means;
        for (int i = 0; i < 6; ++i) {
            means[i] = accumulate(extracted_data.begin(), extracted_data.end(), 0.0, [i](double sum, const array<double, 6>& data_row) { return sum + data_row[i]; })/extracted_data.size();  
        }

        // 3. Calcolo dell'indice di correlazione di Pearson tra le variabili TP2 (indice [0]) e Oil_temperature (indice [5])
        double TP2_oiltemp_corr;
        double num = 0.0, den_1 = 0.0, den_2 = 0.0;
        if(!isnan(means[0]) && !isnan(means[5])) {
            int num_notnan = 0;
            for (int j = 0; j < extracted_data.size(); j++) {
                if(!isnan(extracted_data[j][0]) && !isnan(extracted_data[j][5])) {
                    num += (extracted_data[j][0] - means[0])*(extracted_data[j][5] - means[5]);
                    den_1 += pow((extracted_data[j][0] - means[0]), 2);
                    den_2 += pow((extracted_data[j][5] - means[5]), 2);
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
                for (int j = 0; j < extracted_data.size(); j++) {
                    if (!isnan(extracted_data[j][i])) {
                        accumulator += pow((extracted_data[j][i] - means[i]), 2);
                    } else {
                        nans++;
                    }
                }
                int denominator = extracted_data.size() - nans;
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
                for (int j = 0; j < extracted_data.size(); j++) {
                    if(!isnan(extracted_data[j][i]) && (extracted_data[j][i] < means[i] - thrice_stddevs[i] || extracted_data[j][i] > means[i] + thrice_stddevs[i])) {
                        anomalies_counter++;
                    }
                }
            } else {
                cout << "NaN";
            }
            cout << anomalies_counter << endl;
        }

        // termine della misurazione del tempo impiegato per l'esecuzione di tutto il programma e calcolo del risultato
        auto end_time = high_resolution_clock::now();
        duration<double, std::milli> execution_duration = end_time - start_time;

        cout << endl << "Execution time for the full C++ program (measured from within): " << execution_duration.count() << " milliseconds" << endl;

        emscripten_cancel_main_loop();
    }
}
