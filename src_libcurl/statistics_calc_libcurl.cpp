#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <chrono>
#include <curl/curl.h>
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
using std::min_element;
using std::max_element;
using std::accumulate;
using std::abs;
using std::transform;
using std::getenv;
using nlohmann::json;
using namespace std::chrono;

string url;
size_t callback(char* ptr, size_t size, size_t nmemb, void* userdata);
CURL* addQuery(const string& payload, CURLM* multi_handle, void* buffer, struct curl_slist* headers);

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

    // determinazione dell'indirizzo/nome a cui far riferimento per contattare la porta 8000 su cui è in ascolto il servizio fastAPI
    const char* env_host_addr = getenv("HOST_ADDR");
    string host_address =  env_host_addr ? env_host_addr : "localhost";
    url = "http://" + host_address + ":8000/query";

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
    // Inizializzazione del multi_handle e del contatore per la gestione del ciclo di richieste
    curl_global_init(CURL_GLOBAL_ALL);
    CURLM* multi_handle = curl_multi_init(); 
    if(!multi_handle) {
        cerr << "Could not initialize CURLM handle" << endl;
        return 1;
    }
    CURLMcode mcode;
    int still_running = 0; 

    // dichiarazione del buffer per il salvataggio per il risultato della query, sia in formato JSON che come dato numerico, e dell'easy handle per effettuare la richiesta 
    vector<array<double, 6>> extracted_data;
    vector<double> oilactivation_column;
    string JSON_buffer;
    CURL* easy_handle;

    // preparazione degli headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    CURL *added_handle = addQuery(payload, multi_handle, &JSON_buffer, headers);
    if(!added_handle) {
        cerr << "Handle for the request not found." << endl;
        curl_multi_remove_handle(multi_handle, easy_handle);
        curl_easy_cleanup(easy_handle);
        curl_multi_cleanup(multi_handle);
        curl_slist_free_all(headers);
        return 1;
    }

    // ciclo per la gestione della richiesta in modo asincrono
    int num_sockets;
    do {
        num_sockets = 0;

        mcode = curl_multi_perform(multi_handle, &still_running); 
        if (mcode != CURLM_OK) {
            cerr << "curl_multi_perform() failed." << endl;
            cerr << "Message: " << curl_multi_strerror(mcode) << endl;
            curl_multi_remove_handle(multi_handle, easy_handle);
            curl_easy_cleanup(easy_handle);
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

    // parsing della risposta JSON
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

    curl_global_cleanup();
    cout << endl << "Calculations performed considering " << num_tuples << " tuples." << endl;

    // termine della misurazione del tempo impiegato per l'esecuzione di tutto il programma e calcolo del risultato
    auto end_time = high_resolution_clock::now();
    duration<double, std::milli> execution_duration = end_time - start_time;

    cout << endl << "Execution time for the full C++ program (measured from within): " << execution_duration.count() << " milliseconds" << endl;

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

// funzione per creazione, configurazione e aggiunta a multi_handle di un singolo easy_handle associato ad una richiesta HTTP da eseguire
CURL* addQuery(const string& payload, CURLM* multi_handle, void* buffer, struct curl_slist* headers) {
    CURL* easy_handle = curl_easy_init();
    if(!easy_handle) {
        cerr << "Could not initialize CURL struct for request with payload: " << payload << endl;
        return nullptr;
    }
    CURLcode res;

    // impostazione dell'url
    // Per wsl2: host.docker.internal (caso specifico wsl2 Mauri: 172.21.145.222). Per ambiente unix: 172.17.0.1.
    res = curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_URL option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }

    // impostazione del metodo POST
    res = curl_easy_setopt(easy_handle, CURLOPT_POST, 1L);
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_POST option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }

    // impostazione del corpo della richiesta (JSON)
    res = curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, payload.c_str());
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_POSTFIELDS option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }
    res = curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDSIZE, payload.size());
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_POSTFIELDSIZE option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }

    // impostazione degli headers opportuni
    res = curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, headers);
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_HTTPHEADER option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }

    // impostazione di timeout e limite minimo di velocità di trasmissione in prevenzione di eventuali blocchi del server/ristagni della connessione
    res = curl_easy_setopt(easy_handle, CURLOPT_CONNECTTIMEOUT, 10L); // max 10 s per stabilire la connessione
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_CONNECTTIMEOUT option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }
    res = curl_easy_setopt(easy_handle, CURLOPT_TIMEOUT, 120L); // max 2 min per completare l'intera operazione
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_TIMEOUT option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }
    res = curl_easy_setopt(easy_handle, CURLOPT_LOW_SPEED_LIMIT, 5L); // minimo 5 byte/s
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_LOW_SPEED_LIMIT option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }
    res = curl_easy_setopt(easy_handle, CURLOPT_LOW_SPEED_TIME, 30L); // minimo per 30 s
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_LOW_SPEED_TIME option for request with payload: " << payload << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }
    
    // impostazione di redirezione automatica se indicato dalla risposta ricevuta
    /*
    res = curl_easy_setopt(easy_handle, CURLOPT_FOLLOWLOCATION, 1L);
    if (res != CURLE_OK) {
        cerr << "Could not set CURLOPT_FOLLOWLOCATION option for query: " << query << endl;
        cerr << "Message: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }
    */

    // registrazione della callback, con annesso buffer raccogliere i dati
    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, buffer);

    // esecuzione effettiva della richiesta http con allegata la query
    CURLMcode mres = curl_multi_add_handle(multi_handle, easy_handle);
    if (mres != CURLM_OK) {
        cerr << "Could not add easy handle to multi handle for query: " << payload << endl;
        cerr << "Message: " << curl_multi_strerror(mres) << endl;
        curl_easy_cleanup(easy_handle);
        return nullptr;
    }

    return easy_handle;
}
