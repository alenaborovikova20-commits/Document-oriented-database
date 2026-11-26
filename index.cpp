#include "index.h"
#include <fstream>
#include <iomanip>
#include <sys/stat.h>

static filesystem::path IndexPath(const filesystem::path& path, const string& field){
	const char* hex = "0123456789abcdef";
	string name;
	for (unsigned char c : field){
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9') || c == '_' || c == '-'){
			name += static_cast<char>(c);
		}else{
			name += '%';
			name += hex[c / 16];
			name += hex[c % 16];
		}
	}

	filesystem::path index = path;
	index += "." + name + ".idx";
	return index;
}

static string CollectionVersion(const filesystem::path& path){
	struct stat info{};
	if (stat(path.c_str(), &info) != 0){throw runtime_error("Не удалось проверить версию коллекции");}
	return to_string(info.st_dev) + ":" + to_string(info.st_ino) + ":" + to_string(info.st_size)
		+ ":" + to_string(info.st_mtim.tv_sec) + ":" + to_string(info.st_mtim.tv_nsec)
		+ ":" + to_string(info.st_ctim.tv_sec) + ":" + to_string(info.st_ctim.tv_nsec);
}

static void BuildIndex(BTree& tree, HashTable& table, const string& field){
	for (unsigned int i = 0; i < table.capacity; ++i){
		for (const Node* cur = table.cells[i].node; cur != nullptr; cur = cur->next){
			JsonParser parser(cur->value);
			JsonValue doc = parser.Parse();
			const JsonValue* value = FindField(doc, &field);
			if (value != nullptr && IsIndexKey(*value)){tree.Put(*value, cur->key);}
		}
	}
}

static void SaveIndex(const filesystem::path& path, const string& field, const string& version, const BTree& tree){
	filesystem::path temp = path;
	temp += ".temp";

	try{
		ofstream file(temp);
		if (!file){throw runtime_error("Не удалось открыть временный файл индекса");}
		file << "MiniDBMS-index-1 " << tree.degree << ' ' << version << ' ' << quoted(field) << '\n';
		tree.Save(file);
		file.close();
		if (!file){throw runtime_error("Ошибка записи индекса");}
		filesystem::rename(temp, path);
	}catch (...){
		error_code error;
		filesystem::remove(temp, error);
		throw;
	}
}

static bool ReadIndexHeader(istream& file, const string& field, int& degree, string& version){
	string format, saved_field;
	return (file >> format >> degree >> version >> quoted(saved_field))
		&& format == "MiniDBMS-index-1" && saved_field == field && degree >= 2 && degree <= 64;
}

void CreateIndex(const filesystem::path& path, HashTable& table, const string& input){
	string field = input;
	int degree = 2;
	if (!input.empty() && input[0] == '{'){
		JsonParser parser(input);
		JsonValue query = parser.Parse();
		const string field_name = "field", degree_name = "degree";
		const JsonValue* name = FindField(query, &field_name);
		const JsonValue* order = FindField(query, &degree_name);
		if (name == nullptr || name->type != JsonType::String){
			throw runtime_error("Для create_index нужно строковое поле field");
		}
		field = name->text;
		if (order != nullptr){
			if (order->type != JsonType::Number || order->number < 2 || order->number > 64
				|| order->number != static_cast<int>(order->number)){
				throw runtime_error("Степень B-дерева должна быть целым числом от 2 до 64");
			}
			degree = static_cast<int>(order->number);
		}
	}
	if (field.empty()){throw runtime_error("Укажите имя индексируемого поля");}

	BTree tree(degree);
	BuildIndex(tree, table, field);
	SaveIndex(IndexPath(path, field), field, CollectionVersion(path), tree);
	cout << "Индекс по полю " << field << " создан\n";
}

static bool CanUseIndex(const string& op, const JsonValue& value){
	if (op == "$eq"){return IsIndexKey(value);}
	if (op == "$gt" || op == "$lt"){return value.type == JsonType::Number;}
	if (op != "$in" || value.type != JsonType::Array){return false;}
	for (int i = 0; i < value.array.size; ++i){
		if (!IsIndexKey(value.array.data[i])){return false;}
	}
	return true;
}

static bool FindByIndex(const filesystem::path& path, HashTable& table, const string& field,
	const string& op, const JsonValue& value, HashTable& ids){
	if (!CanUseIndex(op, value)){return false;}

	filesystem::path index = IndexPath(path, field);
	ifstream file(index);
	int degree;
	string version;
	if (!ReadIndexHeader(file, field, degree, version)){return false;}

	BTree tree(degree);
	string current_version = CollectionVersion(path);
	if (version == current_version){
		try{
			tree.Load(file);
		}catch (const runtime_error&){
			return false;
		}catch (const logic_error&){
			return false;
		}
	}else{
		// После изменения коллекции старый индекс перестраивается при первом обращении.
		BuildIndex(tree, table, field);
		try{
			SaveIndex(index, field, current_version, tree);
		}catch (const runtime_error&){
			// Поиск может использовать построенное дерево даже без записи на диск.
		}
	}

	if (op == "$eq"){tree.Find(value, ids);}
	else if (op == "$gt" || op == "$lt"){tree.Range(value, op == "$gt", ids);}
	else{
		for (int i = 0; i < value.array.size; ++i){tree.Find(value.array.data[i], ids);}
	}
	return true;
}

bool FindIndexedIds(const filesystem::path& path, HashTable& table, const JsonValue& query, HashTable& ids){
	for (const JsonNode* field = query.first; field != nullptr; field = field->next){
		const JsonValue& value = *field->value;
		if (field->name == "$or"){continue;}
		if (field->name == "$and"){
			if (value.type != JsonType::Array){continue;}
			for (int i = 0; i < value.array.size; ++i){
				if (FindIndexedIds(path, table, value.array.data[i], ids)){return true;}
			}
			continue;
		}
		if (IsIndexKey(value) && FindByIndex(path, table, field->name, "$eq", value, ids)){return true;}
		if (value.type != JsonType::Object){continue;}

		for (const JsonNode* op = value.first; op != nullptr; op = op->next){
			if (FindByIndex(path, table, field->name, op->name, *op->value, ids)){return true;}
		}
	}
	return false;
}
