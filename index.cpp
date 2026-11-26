#include "index.h"
#include <fstream>
#include <cstdint>

 filesystem::path IndexPath(const filesystem::path& path, const string& field){
	string name;
	for (char c : field){
		switch (c){
			case '%': name += "%%"; break;
			case '/': name += "%s"; break;
			case '\\': name += "%b"; break;
			case '.': name += "%d"; break;
			case '\0': name += "%0"; break;
			default: name += c; break;
		}
	}

	filesystem::path index = path;
	index += "." + name + ".idx";
	return index;
}

string CollectionVersion(const string& content){
	uint64_t hash = 0;
	for (unsigned char c : content){
		hash = hash * 131 + c;
	}
	return "text-" + to_string(content.size()) + "-" + to_string(hash);
}

 void WriteHeaderField(ostream& file, const string& field){
	file.put('"');
	for (char c : field){
		if (c == '"' || c == '\\'){file.put('\\');}
		file.put(c);
	}
	file.put('"');
}

 bool ReadHeaderField(istream& file, string& field){
	char c;
	do{
		if (!file.get(c)){return false;}
	}while (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v');

	if (c != '"'){return false;}

	field.clear();
	while (file.get(c)){
		if (c == '"'){return true;}
		if (c == '\\' && !file.get(c)){return false;}
		field += c;
	}

	return false;
}

 void BuildIndex(BTree& tree, HashTable& table, const string& field){
	for (unsigned int i = 0; i < table.capacity; ++i){
		for (const Node* cur = table.cells.data[i].node; cur != nullptr; cur = cur->next){
			JsonParser parser(cur->value);
			JsonValue doc = parser.Parse();
			const JsonValue* value = FindField(doc, &field);
			if (value != nullptr && IsIndexKey(*value)){tree.Put(*value, cur->key);}
		}
	}
}

 void SaveIndex(const filesystem::path& path, const string& field,
	const string& version, const BTree& tree){
	filesystem::path temp = path;
	temp += ".temp";

	try{
		ofstream file(temp);
		if (!file){throw runtime_error("Не удалось открыть временный файл индекса");}
		file << "MiniDBMS-index-1 " << tree.degree << ' ' << version << ' ';
		WriteHeaderField(file, field);
		file << '\n';
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

 bool ReadIndexHeader(istream& file, const string& field, int& degree, string& version){
	string format, saved_field;
	return (file >> format >> degree >> version) && ReadHeaderField(file, saved_field)
		&& format == "MiniDBMS-index-1" && saved_field == field && degree >= 2 && degree <= 64;
}

void CreateIndex(const filesystem::path& path, HashTable& table,
	const string& input, const string& current_version){
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
			if (order->type != JsonType::Number || !(order->number >= 2 && order->number <= 64)){
				throw runtime_error("Степень B-дерева должна быть целым числом от 2 до 64");
			}

			int whole = order->number;
			if (order->number != whole){
				throw runtime_error("Степень B-дерева должна быть целым числом от 2 до 64");
			}
			degree = whole;
		}
	}
	if (field.empty()){throw runtime_error("Укажите имя индексируемого поля");}

	BTree tree(degree);
	BuildIndex(tree, table, field);
	SaveIndex(IndexPath(path, field), field, current_version, tree);
	cout << "Индекс по полю " << field << " создан\n";
}

 bool CanUseIndex(const string& op, const JsonValue& value){
	if (op == "$eq"){return IsIndexKey(value);}
	if (op == "$gt" || op == "$lt"){return value.type == JsonType::Number;}
	if (op != "$in" || value.type != JsonType::Array){return false;}

	for (int i = 0; i < value.array.size; ++i){
		if (!IsIndexKey(value.array.data[i])){return false;}
	}
	return true;
}

 bool FindByIndex(const filesystem::path& path, HashTable& table, const string& field,
	const string& op, const JsonValue& value, HashTable& ids, const string& current_version){
	if (!CanUseIndex(op, value)){return false;}

	filesystem::path index = IndexPath(path, field);
	ifstream file(index);
	int degree;
	string version;
	if (!ReadIndexHeader(file, field, degree, version)){return false;}

	BTree tree(degree);
	if (version == current_version){
		try{
			tree.Load(file);
		}catch (const runtime_error&){
			return false;
		}catch (const logic_error&){
			return false;
		}
	}else{
		BuildIndex(tree, table, field);
		try{
			SaveIndex(index, field, current_version, tree);
		}catch (const runtime_error&){
			// При ошибке записи поиск продолжится по дереву в памяти.
		}
	}

	if (op == "$eq"){
		tree.Find(value, ids);
	}else if (op == "$gt" || op == "$lt"){
		tree.Range(value, op == "$gt", ids);
	}else{
		for (int i = 0; i < value.array.size; ++i){
			tree.Find(value.array.data[i], ids);
		}
	}

	return true;
}

bool FindIndexedIds(const filesystem::path& path, HashTable& table, const JsonValue& query,
	HashTable& ids, const string& current_version){
	for (const JsonNode* field = query.first; field != nullptr; field = field->next){
		const JsonValue& value = *field->value;

		if (field->name == "$or"){continue;}

		if (field->name == "$and"){
			if (value.type != JsonType::Array){continue;}

			for (int i = 0; i < value.array.size; ++i){
				if (FindIndexedIds(path, table, value.array.data[i], ids, current_version)){
					return true;
				}
			}
			continue;
		}

		if (IsIndexKey(value) &&
			FindByIndex(path, table, field->name, "$eq", value, ids, current_version)){
			return true;
		}

		if (value.type != JsonType::Object){continue;}

		for (const JsonNode* op = value.first; op != nullptr; op = op->next){
			if (FindByIndex(path, table, field->name, op->name, *op->value, ids, current_version)){
				return true;
			}
		}
	}

	return false;
}