#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <emscripten/fetch.h>
#include <emscripten.h>
#include <cstdio>
#include "external/nlohmann/json.hpp"
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::array;
using std::vector;
using std::tuple;
using std::string;
using std::to_string;
using std::replace;
using std::find;
using std::stoi;
using std::stod;
using std::nan;
using std::isnan;
using std::abs;
using std::transform;
using nlohmann::json;

// QUESTO FILE E' GRAVEMENTE INCOMPLETO E ATTUALMENTE NON FUNZIONA E NON HA SENSO

size_t callback_success(char* ptr, size_t size, size_t nmemb, void* userdata);
size_t callback_failure(char* ptr, size_t size, size_t nmemb, void* userdata);
size_t callback_timeout(void* arg);

int still_running = 4;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cerr << "Use: " << argv[0] << " <number of tuples to consider>" << endl;
        return 1;
    }

    int num_tuples = stoi(argv[1]);
    if (num_tuples < 1) {
        cerr << "Error: number of tuples must be a positive integer." << endl;
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
    string sql_data_limited = "SELECT timestamp, TP2, TP3, H1, DV_pressure, Reservoirs, Oil_temperature, Oil_level FROM aircompressor_data" + limit_clause;

    // preparazione delle variabili per trasformare le query in payloads pronti per essere trasmessi come corpo di una richiesta http
    vector<string> payloads;
    json body;

    // 1. 
    string sql_reservois_TP3_diff = 
    "SELECT AVG(Reservoirs - TP3) AS avg_diff, "
    "MAX(Reservoirs - TP3) AS max_diff, "
    "MIN(Reservoirs - TP3) AS min_diff "
    "FROM (" + sql_data_limited + ") AS limited_data;";
    body["query"] = sql_reservois_TP3_diff;
    payloads.push_back(body.dump());

    // 2. 
    string sql_oillevel_activation = 
    "WITH limited_data AS (" + sql_data_limited + ") "
    "SELECT CAST(100.0 * ("
        "(SELECT COUNT(Oil_level) FROM limited_data WHERE Oil_level = 1.0) * 1.0/"
        "(SELECT COUNT(Oil_level) FROM limited_data)) AS DOUBLE) AS oillevel_activation_percentage;";
    body["query"] = sql_oillevel_activation;
    payloads.push_back(body.dump());

    // 3/4/5.
    string sql_data = "SELECT TP2, TP3, H1, DV_pressure, Reservoirs, Oil_temperature FROM aircompressor_data" + limit_clause + ";";
    body["query"] = sql_data;
    payloads.push_back(body.dump());

    string sql_means = "SELECT AVG(TP2), AVG(TP3), AVG(H1), AVG(DV_pressure), AVG(Reservoirs), AVG(Oil_temperature) FROM (" + sql_data_limited + ") AS limited_data;";
    body["query"] = sql_means;
    payloads.push_back(body.dump());

    // ESECUZIONE DELLE QUERIES tramite richiesta http ad app.py con fastAPI, che si interfaccia poi con il database server MySQL
    // Inizializzazione delle strutture necessarie ad eseguire le richieste http tramite l'API fetch di emscripten e impostazione delle configurazioni adeguate
    emscripten_fetch_attr_t fetch_attr;
    emscripten_fetch_attr_init(&fetch_attr);

    strcpy(fetch_attr.requestMethod, "POST");
    static const char* headers[] = {
        "Content-Type: application/json",
        "Accept: application/json",
    };
    fetch_attr.requestHeaders = headers;
    fetch_attr.requestHeadersCount = 2;

    fetch_attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    fetch_attr.onsuccess = callback_success;
    fetch_attr.onerror = callback_failure;

    // Chiamate asincrone
    array<int, 4> timeout_ids;
    for (int i = 0; i < 4; i++) {
        fetch_attr.requestData = payloads[i];
        fetch_attr.requestDataSize = strlen(fetch_attr.requestData);
        emscripten_fetch_t* fetch = emscripten_fetch(&fetch_attr, "http://172.21.145.222:8000/query");
        int timeout_ids[i] = emscripten_set_timeout(callback_timeout, 120000, fetch)
    }

    // dichiarazione dei buffer per il salvataggio dei risultati delle query 
    // (sia come stringhe per accogliere inizialmente i dati in formato JSON, sia come buffer dei giusti tipi di dati per poterli usare nelle computazioni successive)
    array<double, 3> extracted_reservoisTP3diff;
    double extracted_oillevelactivation;
    vector<array<double, 6>> extracted_data;
    array<double, 6> extracted_means;

    // inserimento delle queries e dei relativi buffer in una struttura unica per comodità di utilizzo
    vector<string> JSON_buffers(4);
    // creazione e aggiunta dei vari easy_handles associati alle singole query
    vector<emscripten_fetch_attr_t> easy_handles; 
    // preparazione degli headers, comuni a tutte le richieste
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    for (int i = 0; i < 4; i++) {
        CURL *added_handle = addQuery(payloads[i], multi_handle, &JSON_buffers[i], headers);
        if(!added_handle) {
            cerr << "Handle not found for request with payload: " << payloads[i] << endl;
            for (CURL* easy_handle : easy_handles) {
                curl_multi_remove_handle(multi_handle, easy_handle);
                curl_easy_cleanup(easy_handle);
            }
            curl_multi_cleanup(multi_handle);
            curl_slist_free_all(headers);
            return 1;
        }
        easy_handles.push_back(added_handle);
    }

    // ciclo per la gestione delle richieste asincrone
    int num_sockets;
    do {
        num_sockets = 0;

        mcode = curl_multi_perform(multi_handle, &still_running); 
        if (mcode != CURLM_OK) {
            cerr << "curl_multi_perform() failed." << endl;
            cerr << "Message: " << curl_multi_strerror(mcode) << endl;
            for (CURL* easy_handle : easy_handles) {
                curl_multi_remove_handle(multi_handle, easy_handle);
                curl_easy_cleanup(easy_handle);
            }
            curl_multi_cleanup(multi_handle);
            curl_slist_free_all(headers);
            return 1;
        }

        mcode = curl_multi_poll(multi_handle, NULL, 0, 1000, &num_sockets);
        if (mcode != CURLM_OK) {
            cerr << "curl_multi_poll() failed." << endl;
            cerr << "Message: " << curl_multi_strerror(mcode) << endl;

            break;
        }

        CURLMsg* message = nullptr;
        int remaining_msg = 0; 

        while ((message = curl_multi_info_read(multi_handle, &remaining_msg))) {
            if (message->msg == CURLMSG_DONE) {
                CURL* retrieved_easy_handle = message->easy_handle;
                if (message->data.result != CURLE_OK) {
                    cerr << "One of the requests failed." << endl;
                    cerr << "Message: " << curl_easy_strerror(message->data.result) << endl << endl;
                    return 1;
                }
                curl_multi_remove_handle(multi_handle, retrieved_easy_handle);
                curl_easy_cleanup(retrieved_easy_handle);
            }
        }
    }
    while (still_running > 0);
    curl_multi_cleanup(multi_handle);
    curl_slist_free_all(headers);

    // parsing delle risposte JSON
    json parsed_data;

    try {
        // 1a query
        parsed_data = json::parse(JSON_buffers[0]);
        extracted_reservoisTP3diff[0] = abs(parsed_data[0]["avg_diff"].get<double>());
        extracted_reservoisTP3diff[1] = abs(parsed_data[0]["max_diff"].get<double>());
        extracted_reservoisTP3diff[2] = abs(parsed_data[0]["min_diff"].get<double>());

        // 2a query
        parsed_data = json::parse(JSON_buffers[1]);
        extracted_oillevelactivation = parsed_data[0]["oillevel_activation_percentage"].get<double>();

        // 3a query
        parsed_data = json::parse(JSON_buffers[2]);
        for (const auto& row : parsed_data) {
            array<double, 6> parsed_row;
            parsed_row[0] = row["TP2"].get<double>();
            parsed_row[1] = row["TP3"].get<double>();
            parsed_row[2] = row["H1"].get<double>();
            parsed_row[3] = row["DV_pressure"].get<double>();
            parsed_row[4] = row["Reservoirs"].get<double>();
            parsed_row[5] = row["Oil_temperature"].get<double>();
            extracted_data.push_back(parsed_row);
        }

        // 4a query
        parsed_data = json::parse(JSON_buffers[3]);
        extracted_means[0] = parsed_data[0]["AVG(TP2)"].get<double>();
        extracted_means[1] = parsed_data[0]["AVG(TP3)"].get<double>();
        extracted_means[2] = parsed_data[0]["AVG(H1)"].get<double>();
        extracted_means[3] = parsed_data[0]["AVG(DV_pressure)"].get<double>();
        extracted_means[4] = parsed_data[0]["AVG(Reservoirs)"].get<double>();
        extracted_means[5] = parsed_data[0]["AVG(Oil_temperature)"].get<double>();
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

    // STAMPE SU STDOUT ED EVENTUALI COMPUTAZIONI ULTERIORI
    cout << "### DIFFERENCES RESERVOIRS - TP3 (avg, max, min) ###" << endl;
    for (const auto& value : extracted_reservoisTP3diff) {
        cout << value << "\t";
    }
    cout << endl << "\n### OIL LEVEL ACTIVATION PERCENTAGE ###" << endl;
    cout << extracted_oillevelactivation << "%" << endl;

    // Calcolo dell'indice di correlazione di Pearson tra le variabili TP2 (indice [0]) e Oil_temperature (indice [5])
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
    cout << "\n### CORRELATION BETWEEN TP2 AND OIL TEMPERATURE ###" << endl << TP2_oiltemp_corr << endl;

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
    cout << "\n### AVERAGE, VARIANCE, STANDARD DEVIATION ###" << endl;
    for(int i = 0; i < variances.size(); i++) {
        cout << column_names[i] << ": " << extracted_means[i] << ", " << variances[i] << ", " << sqrt(variances[i]) << endl;
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
        cout << column_names[i] << ": ";
        anomalies_counter = 0;
        if(!isnan(extracted_means[i]) && !isnan(thrice_stddevs[i])) {
            for (int j = 0; j < extracted_data.size(); j++) {
                if(!isnan(extracted_data[j][i]) && (extracted_data[j][i] < extracted_means[i] - thrice_stddevs[i] || extracted_data[j][i] > extracted_means[i] + thrice_stddevs[i])) {
                    anomalies_counter++;
                }
            }
        } else {
            cout << "NaN";
        }
        cout << anomalies_counter << endl;
    }

    curl_global_cleanup();
    cout << endl << "Calculations performed considering " << num_tuples << " tuples." << endl;

    return 0;
}

// funzione di callback per raccogliere i dati in formato JSON contenuti nella risposta inviata da fastAPI
size_t callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t num_bytes_received = size * nmemb;

    string* buffer = nullptr; 
    if (userdata) {
        buffer = static_cast<string*>(userdata);
        buffer->append(ptr, num_bytes_received);
    }

    return num_bytes_received; 
}

void callback_success(emscripten_fetch_t *fetch) {
    std::cout << "Download succeeded, " << fetch->numBytes << " bytes\n";
    std::cout << "Response: " << std::string(fetch->data, fetch->numBytes) << std::endl;
    still_running--;
    emscripten_clear_timeout(timeout_id); // poi vedo da dove prendo timeout_id
    emscripten_fetch_close(fetch);
}