#pragma once
#include <string>
#include "array.h"

using namespace std;

enum class JsonType{
    Object,
    Array,
    String,
    Number,
    Bool,
    Null
};

struct JsonNode;
struct JsonValue{
    JsonType type = JsonType::Null; 

    string text;
    double number = 0.0;
    bool boolean = false;
    bool isNull = false;
    Array<JsonValue> array;

    JsonNode* first = nullptr;
};



struct JsonNode{
    string name;
    JsonValue* value = nullptr;
    JsonNode* next = nullptr;
};

class JsonParser{
public:
    JsonParser(const string& input_str);
    JsonValue Parse();

private:
    string input;
    size_t pos = 0;
    void SkipSpace();
    char Peek();
    char Get();
    JsonValue ParseValue();
    JsonValue ParseString();
    JsonValue ParseNumber();
    JsonValue ParseBool();
    JsonValue ParseArray();
    JsonValue ParseObject();
    JsonValue ParseNull();
};



JsonValue* FindField(const JsonValue& object, const string* name);

string ToJson(const JsonValue& value);