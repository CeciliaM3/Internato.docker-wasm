#include <emscripten/fetch.h>
#include <iostream>

void downloadSucceeded(emscripten_fetch_t *fetch) {
    std::cout << "Download succeeded, " << fetch->numBytes << " bytes\n";
    std::cout << "Response: " << std::string(fetch->data, fetch->numBytes) << std::endl;
    emscripten_fetch_close(fetch);
}

void downloadFailed(emscripten_fetch_t *fetch) {
    std::cerr << "Download failed with HTTP status code: " << fetch->status << std::endl;
    emscripten_fetch_close(fetch);
}

int main() {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;

    emscripten_fetch(&attr, "http://172.17.0.1:8000/records/10");

    return 0;
}