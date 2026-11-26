#include "command.h"
#include "json_parser.h"
#include "collection.h"
#include "index.h"




size_t NextSymbol(const string& text, size_t pos){
	++pos;
	while (pos < text.size()){
		unsigned char byte = text[pos];
		if (byte < 128 || byte > 191){break;}
		++pos;
	}
	return pos;
}

bool MatchesLike(const string& text, const string& pattern){
	size_t i = 0;
	size_t j = 0;
	size_t after_percent = string::npos;
	size_t percent_end = 0;

	while (i < text.size()){
		if (j < pattern.size() && pattern[j] == '%'){
			after_percent = ++j;
			percent_end = i;
			continue;
		}

		size_t next_i = NextSymbol(text, i);
		if (j < pattern.size()){
			size_t next_j = NextSymbol(pattern, j);
			if (pattern[j] == '_' ||
				(next_i - i == next_j - j &&
				text.compare(i, next_i - i, pattern, j, next_j - j) == 0)){
				i = next_i;
				j = next_j;
				continue;
			}
		}

		if (after_percent == string::npos){return false;}

		// Последний % поглощает ещё один символ.
		percent_end = NextSymbol(text, percent_end);
		i = percent_end;
		j = after_percent;
	}

	while (j < pattern.size() && pattern[j] == '%'){
		++j;
	}

	return j == pattern.size();
}


bool EqualValues(const JsonValue& current, const JsonValue& expected){
	if (current.type != expected.type){return false;}

	switch (current.type)
	{
		case JsonType::String: {return current.text == expected.text;}
		case JsonType::Number: {return current.number == expected.number;}
		case JsonType::Bool: {return current.boolean == expected.boolean;}
		case JsonType::Null: {return true;}
	default:
		return false;
	}
}


bool MatchesFilter(const JsonValue& doc, const JsonValue& query){
	const JsonNode* condition = query.first;

	while (condition != nullptr){
				if (condition->name == "$or"){
			const JsonValue* filters = condition->value;

			if (filters->type != JsonType::Array || filters->array.size == 0){
				return false;
			}

			bool matched = false;
			for (int i = 0; i < filters->array.size; ++i){
				const JsonValue& filter = filters->array.data[i];
				if (filter.type != JsonType::Object){return false;}

				if (!matched && MatchesFilter(doc, filter)){
					matched = true;
				}
			}

			if (!matched){return false;}

			condition = condition->next;
			continue;
		}
				if (condition->name == "$and"){
			const JsonValue* filters = condition->value;
			if (filters->type != JsonType::Array || filters->array.size == 0){
				return false;
			}

			for (int i = 0; i < filters->array.size; ++i){
				const JsonValue& filter = filters->array.data[i];
				if (filter.type != JsonType::Object){return false;}
				if (!MatchesFilter(doc, filter)){return false;}
			}

			condition = condition->next;
			continue;
		}
		const JsonValue* actual = FindField(doc, &condition->name);
		if (actual == nullptr){return false;}

		const JsonValue* expected = condition->value;

		

		if (expected->type == JsonType::Object){
			JsonNode* cur = expected->first;
			if (cur == nullptr){return false;}
			while(cur  != nullptr){
				string operation = cur->name;
				JsonValue* value = cur->value;

				if (operation == "$eq"){
					if (!EqualValues(*actual, *value)){return false;}
				} else if (operation == "$gt" || operation == "$lt"){
					if (actual->type !=  cur->value->type || cur->value->type != JsonType::Number){
						return false;
					}
					if (operation == "$gt" && actual->number <= value->number){return false;}
					if (operation == "$lt" && actual->number >= value->number){return false;}
				} else if (operation == "$in"){
					if (value->type != JsonType::Array){return false;}
					bool isFind = false;
					for (int i = 0; i < value->array.size; ++i){
						if (EqualValues(*actual, value->array.data[i])){
							isFind = true;
							break;
						}
					}
					if (isFind == false){return false;}
								}else if (operation == "$like"){
					if (actual->type != JsonType::String || value->type != JsonType::String){
						return false;
					}
					if (!MatchesLike(actual->text, value->text)){return false;}
				}else{
					return false;
				}

				cur = cur->next;
			}
		}else{
			if (!EqualValues(*actual, *expected)){return false;};
		}

		condition = condition->next;
	}
	return true;
}



string GenerateID(HashTable& table){
	int i = 1;
	string id = to_string(i);

	while (table.Get(id) != nullptr){
		++i;
		id = to_string(i);
	}

	return id;
}


int ExecuteDelete(HashTable& table, const filesystem::path& collectionPath, const JsonValue& query){
	if (query.type != JsonType::Object){
		cerr << "Для delete нужен JSON-объект\n";
		return 1;
	}

	int removed = 0;

	for (unsigned int i = 0; i < table.capacity; ++i){
		Node* cur = table.cells[i].node;

		while (cur != nullptr){
			Node* next = cur->next;

			JsonParser parser(cur->value);
			JsonValue doc = parser.Parse();

			if (MatchesFilter(doc, query)){
				table.Remove(cur->key);
				++removed;
			}

			cur = next;
		}
	}

	if (removed > 0){
		SaveCollection(collectionPath, table);
	}

	cout << "Удалено документов: " << removed << '\n';
	return 0;
}

int ExecuteInsert(HashTable& table, const filesystem::path& collectionPath, JsonValue& query){
	if (query.type != JsonType::Object){
		cerr << "Для insert нужен JSON-объект\n";
		return 1;
	}

	const string id_name = "_id";
	const JsonValue* field = FindField(query, &id_name);
	string id;

	if (field == nullptr){
		id = GenerateID(table);

		JsonValue id_value;
		id_value.type = JsonType::String;
		id_value.text = id;

		query.first = new JsonNode{id_name, new JsonValue(id_value), query.first};
	}else{
		if (field->type != JsonType::String){
			cerr << "_id должен быть строкой\n";
			return 1;
		}

		id = field->text;
		if (table.Get(id) != nullptr){
			cerr << "Документ с _id " << id << " уже существует\n";
			return 1;
		}
	}

	table.Put(id, ToJson(query));
	SaveCollection(collectionPath, table);

	cout << "Документ успешно добавлен\n";
	return 0;
}


static void PrintMatch(const Node& node, const JsonValue& query){
	JsonParser parser(node.value);
	JsonValue doc = parser.Parse();
	if (MatchesFilter(doc, query)){
		cout << node.value << '\n';
	}
}

int ExecuteFind(HashTable& table, const JsonValue& query, const filesystem::path& collectionPath){
	if (query.type != JsonType::Object){
		cerr <<  "Для find нужен JSON объект" << endl;
		return 1;
	}
	
	HashTable ids(31);
	ids.InitHashTable(16);
	if (FindIndexedIds(collectionPath, table, query, ids)){
		for (unsigned int i = 0; i < ids.capacity; ++i){
			for (const Node* cur = ids.cells[i].node; cur != nullptr; cur = cur->next){
				const Node* doc = table.Get(cur->key);
				if (doc != nullptr){PrintMatch(*doc, query);}
			}
		}
		return 0;
	}

	for (unsigned int i = 0; i < table.capacity; ++i){
		for (const Node* cur = table.cells[i].node; cur != nullptr; cur = cur->next){
			PrintMatch(*cur, query);
		}
	}
	return 0;
}


int ExecuteCommand(const string& command, const string& input, HashTable& table, const filesystem::path& collectionPath){
	if (command == "create_index"){
		CreateIndex(collectionPath, table, input);
		return 0;
	}

    JsonParser queryParser(input);
    JsonValue query = queryParser.Parse();

    if (command == "insert"){
        return ExecuteInsert(table, collectionPath, query);
    } else if (command == "find"){
        return ExecuteFind(table, query, collectionPath);
    } else if (command == "delete"){
        return ExecuteDelete(table, collectionPath, query);
    } else {
        cerr << "Неизвестная команда" << command << endl;
		return 1; 
    }
}