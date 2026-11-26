#include "hash_table.h"
#include "json_parser.h"
#include "command.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <exception>

using namespace std;

int main(int argc, char** argv){
	if (argc != 5){
		cerr << "Формат ввода: ./no_sql_dbms {Название БД} "
		     << "{Коллекция} {Команда} '{JSON}'\n";
		return 1;
	}

	string db = argv[1];
	string collection = argv[2];
	collection += ".json";
	string command = argv[3];
	string input = argv[4];


	try{
		filesystem::path collectionPath = filesystem::path(db) / collection;
		ifstream file(collectionPath);
		if (!file.is_open()){
			cerr << "Не удалось открыть файл коллекции " << collectionPath << '\n';
			return 1;
		}

		string line, res;
		while (getline(file, line)){
			res += line;
			res += '\n';
		}

		if (!file.eof() || file.bad()){
			cerr << "Ошибка при чтении файла коллекции\n";
			return 1;
		}

		JsonParser parser(res);
		JsonValue root = parser.Parse();

		if (root.type != JsonType::Object){
			cerr << "Ожидался JSON-объект в корне файла\n";
			return 1;
		}

		HashTable table(31);
		table.InitHashTable(16);

		const string id_name = "_id";

		for (const JsonNode* field = root.first; field != nullptr; field = field->next){
			const JsonValue* doc = field->value;
			if (doc == nullptr || doc->type != JsonType::Object){
				cerr << "Документ с ключом " << field->name
				     << " должен быть JSON-объектом\n";
				return 1;
			}

			const JsonValue* id = FindField(*doc, &id_name);
			if (id == nullptr || id->type != JsonType::String){
				cerr << "У документа с ключом " << field->name
				     << " должен быть строковый _id\n";
				return 1;
			}

			if (id->text != field->name){
				cerr << "Ключ " << field->name << " не совпадает с _id "
				     << id->text << '\n';
				return 1;
			}

			if (table.Get(id->text) != nullptr){
				cerr << "Поавторяющийся id" << id->text << endl;
				return 1;
			}
			table.Put(id->text, ToJson(*doc));
		}
		return ExecuteCommand(command, input, table, collectionPath);
	}
	catch (const exception& error){
		cerr << "Ошибка при загрузке коллекции: " << error.what() << '\n';
		return 1;
	}
}