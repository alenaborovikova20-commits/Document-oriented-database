#include "collection.h"
#include "json_parser.h"
#include <fstream>
#include <stdexcept>

using namespace std;

void SaveCollection(const filesystem::path&  path, const HashTable& table){
    filesystem::path temp_path = path;
    temp_path += ".temp";

    ofstream file(temp_path);
    if (!file.is_open()){
        throw runtime_error("Не удалось открыть временный файл коллекции");
    }

    file << '{';
    bool first = true;

    for (unsigned int i = 0; i < table.capacity; ++i){
        Node* cur = table.cells.data[i].node;
        while (cur != nullptr){
            if (!first) {file << ',';}
            JsonValue key;
            key.type = JsonType::String;
            key.text = cur->key;
            file << "\n\t" << ToJson(key) << ": " << cur->value;
            first = false;
            cur = cur->next;
        }
    }
    file << "\n}\n";
    file.close();

    if (!file){
        throw runtime_error("Ошибка запсии коллекции");
    }
    filesystem::rename(temp_path, path);
}