#include "json_parser.h"
#include <cctype>
#include <stdexcept>
using namespace std;

JsonParser::JsonParser(const string& input_str) : input(input_str){}

JsonValue JsonParser::Parse(){
	SkipSpace();
	JsonValue val = ParseValue();
	SkipSpace();

	if (pos != input.size()){throw runtime_error("Лишние символы после JSON");}
	return val;
}

JsonValue JsonParser::ParseValue(){
	SkipSpace();
	unsigned char c = Peek();

	if (c == '{'){return ParseObject();}
	if (c == '['){return ParseArray();}
	if (c == '"'){return ParseString();}
	if (c == 't' || c == 'f'){return ParseBool();}
	if (c == 'n'){return ParseNull();}
	if (c == '-' || isdigit(c)){return ParseNumber();}

	throw runtime_error("Ожидалось JSON-значение");
}


void JsonParser::SkipSpace(){
	while (pos < input.size()){
		unsigned char c = input[pos];
		if (!isspace(c)){break;}
		++pos;}
}


char JsonParser::Peek(){
	if (pos >= input.size()){throw runtime_error("Неожиданный конец JSON");}
	return input[pos];
}

char JsonParser::Get(){
	char c = Peek();
	++pos;
	return c;
}

JsonValue JsonParser::ParseString(){
	if (Get() != '"'){throw runtime_error("Ожидалась кавычка");}

	JsonValue result;
	result.type = JsonType::String;

	while (true){
		char c = Get();
		if (c == '"'){return result;}
		if (c == '\\'){
			switch (Get()){
				case '"': c = '"'; break;
				case '\\': c = '\\'; break;
				case '/': c = '/'; break;
				case 'b': c = '\b'; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				default: throw runtime_error("Неизвестная последовательность");
			}
		}
		result.text += c;
	}
}


JsonValue JsonParser::ParseNumber(){
	string number;
	JsonValue result;
	result.type = JsonType::Number;
	if (Peek() == '-'){number += Get();}

	size_t start = pos;
	while (pos < input.size()){
		unsigned char c = input[pos];
		if (!isdigit(c)){break;}
		number += Get();}
	if (pos == start){throw runtime_error("Ожидалась цифра");}
	if (pos < input.size() && input[pos] == '.'){
		number += Get();
		start = pos;

		while (pos < input.size()){
			unsigned char c = input[pos];
			if (!isdigit(c)){break;}
			number += Get();
		}
		if (pos == start){throw runtime_error("Ожидалась цифра после точки");}
	}
	result.text = number;
	result.number = stod(number);
	return result;}


JsonValue JsonParser::ParseBool(){
	JsonValue result;
	result.type = JsonType::Bool;

	if (input.substr(pos, 4) == "true"){
		result.boolean = true;
		pos += 4;
		return result;
	}
	if (input.substr(pos, 5) == "false"){
		result.boolean = false;
		pos += 5;
		return result;
	}
	throw runtime_error("Ожидалось true || false");
}

JsonValue JsonParser::ParseNull(){
	JsonValue result;
	result.type = JsonType::Null;

	if (input.substr(pos, 4) == "null"){
		result.isNull = true;
		pos += 4;
		return result;
	}
	throw runtime_error("Ожидалось null");
}

JsonValue JsonParser::ParseArray(){
	Get();
	JsonValue result;
	result.type = JsonType::Array;

	SkipSpace();
	if (Peek() == ']'){
		Get();
		return result;
	}

	while (true){
		MPUSH(&result.array, ParseValue());
		SkipSpace();

		char c = Get();
		if (c == ']'){
            return result;}
		if (c != ','){throw runtime_error("Ожидалась запятая или ]");}
	}
}



JsonValue JsonParser::ParseObject(){
	Get();
	JsonValue result;
	result.type = JsonType::Object;
    JsonNode* last = nullptr;

	SkipSpace();
	if (Peek() == '}'){
		Get();
		return result;}

	while (true){
		SkipSpace();
		string name = ParseString().text;
		SkipSpace();
		if (Get() != ':'){throw runtime_error("Ожидалось двоеточие");}
        JsonNode* node = new JsonNode{name, nullptr, nullptr};
        if (last == nullptr){result.first = node;}
        else {last->next = node;}

		last = node;
		node->value =  new JsonValue(ParseValue());
		SkipSpace();

		char c = Get();
		if (c == '}'){return result;}
		if (c != ','){throw runtime_error("Ожидалась запятая или }");}
	}
}


JsonValue* FindField(const JsonValue& object, const string* name){
	if (object.type != JsonType::Object || name == nullptr){return nullptr;}
	JsonNode* cur = object.first;
	while (cur != nullptr){
		if (cur->name == *name){
			return cur->value;
		}
		cur = cur->next;
	}
	return nullptr;
}


string ToJson(const JsonValue& value){
    string result;
    switch (value.type)
    {
    case JsonType::String:{
		result = '"';
		for (unsigned char c : value.text){
			switch (c){
				case '"': result += "\\\""; break;
				case '\\': result += "\\\\"; break;
				case '\b': result += "\\b"; break;
				case '\f': result += "\\f"; break;
				case '\n': result += "\\n"; break;
				case '\r': result += "\\r"; break;
				case '\t': result += "\\t"; break;
				default:
					result += c; 
					break;
					}
			}
			result += '"';
			break;
		}		
    case JsonType::Bool: {
        if (value.boolean == true){result = "true";}
        else {result = "false";}
        break;}
    case JsonType::Null: {
        result = "null";
        break;}
    case JsonType::Array: {
		result = '[';
		for (int i = 0; i < value.array.size; ++i){
			if (i != 0) {result += ',';}
			result += ToJson(value.array.data[i]);
		}
		result += ']';
        break;}
    case JsonType::Number: {
        result = value.text;
        break;}
    case JsonType::Object: {
		result = '{';
		for (JsonNode* field = value.first; field != nullptr; field = field->next){
			if (field != value.first){result += ',';}
			JsonValue name;
			name.type = JsonType::String;
			name.text = field->name;

			result += ToJson(name);
			result += ':';
			result += ToJson(*field->value);
		}
		result += '}';
        break;}
    default:
        break;
    }
	return result;

}