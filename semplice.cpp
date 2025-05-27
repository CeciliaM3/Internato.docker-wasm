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
using std::atomic;
using nlohmann::json;

struct TimeoutContext {
    string *payload; 
    bool valid = true; 
};

struct RequestContext {
    TimeoutContext *timeout_ctx;
    string *JSON_buffer;
};

int num_tuples;
atomic<int> completed_requests = 0; 
atomic<bool> timeout_triggered = false; 
vector<string> JSON_buffers(4);
vector<emscripten_fetch_t*> fetch_structs(4, nullptr);

void callback_success(emscripten_fetch_t* fetch);
void callback_failure(emscripten_fetch_t* fetch);
void callback_timeout(void* ctxvoid);
void main_loop();

void downloadSucceeded(emscripten_fetch_t *fetch) {
    std::cout << "Download succeeded, " << fetch->numBytes << " bytes\n";
    std::cout << "Response: " << std::string(fetch->data, fetch->numBytes) << std::endl;
    emscripten_fetch_close(fetch);
}

void downloadFailed(emscripten_fetch_t *fetch) {
    std::cerr << "Download failed with HTTP status code: " << fetch->status << std::endl;
    emscripten_fetch_close(fetch);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Use: " << argv[0] << " <number of tuples to consider>" << endl;
        return 1;
    }

    num_tuples = stoi(argv[1]);
    if (num_tuples < 1) {
        cerr << "Error: number of tuples must be a positive integer." << endl;
        return 1;
    }

    // determinazione dell'indirizzo/nome a cui far riferimento per contattare la porta 8000 su cui è in ascolto il servizio fastAPI
    const char* env_host_addr = getenv("HOST_ADDR");
    string host_address =  env_host_addr ? env_host_addr : "localhost";
    string url = "http://" + host_address + ":8000/query";

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

    emscripten_fetch_attr_t fetch_attr;
    emscripten_fetch_attr_init(&fetch_attr);
    strcpy(fetch_attr.requestMethod, "POST");

    /*
    static const char* headers[] = {
        "Content-Type: application/json",
        "Accept: application/json",
        nullptr
    };
    fetch_attr.requestHeaders = headers;
    */

    fetch_attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    fetch_attr.onsuccess = downloadSucceeded;
    fetch_attr.onerror = downloadFailed;

    // Chiamate asincrone
    vector<TimeoutContext> timeout_contexts(4);
    vector<RequestContext> request_contexts(4);

    fetch_attr.requestData = payloads[0].c_str();
    fetch_attr.requestDataSize = payloads[0].size();
    emscripten_fetch(&fetch_attr, "http://172.21.145.222:8000/query");

    return 0;
}