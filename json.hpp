#pragma once
#include <string>
#include <string_view>
#include <vector>

enum JsonType{
    JSON_NULL,
    JSON_FALSE,
    JSON_TRUE,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
};

struct JsonContext{
    std::u8string_view json;
};

class JsonValue
{
public:
    JsonValue() : number(0.0), type(JSON_NULL) {}
    JsonValue(JsonType t) : number(0.0), type(t) {}
    JsonValue(double n) : number(n), type(JSON_NUMBER) {}
    JsonValue(const char8_t *s): number(0.0), text(s), type(JSON_STRING) {}
    JsonValue(const std::vector<JsonValue> v) : number(0.0), array(v), type(JSON_ARRAY) {}
    bool operator==(const JsonValue& ) const = default;

    JsonType get_type();
    double get_number();
    std::u8string& get_string();
    std::vector<JsonValue>& get_array();

    void set_type(JsonType);
    void set_number(double);
    void set_string(const std::u8string&);
    void set_array(const std::vector<JsonValue>&);


private:
    double number;
    std::u8string text;
    std::vector<JsonValue> array;
    JsonType type;
};

// Parse result
enum
{
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,
    PARSE_INVALID_VALUE,
    PARSE_INVALID_STRING_END,
    PARSE_INVALID_STRING_CHAR,
    PARSE_INVALID_STRING_ESCAPE,
    PARSE_ROOT_NOT_SINGULAR,
    PARSE_INVALID_UNICODE_HEX,
    PARSE_INVALID_UNICODE_SURROGATE,
    PARSE_INVAID_ARRAY_END,
    PARSE_EXTRA_ARRAY_SEPARATOR,
};

int json_parse(JsonValue &, std::u8string_view);
void json_parse_whitespace(JsonContext&);
int json_parse_value(JsonContext&, JsonValue&);
int json_parse_literal(JsonContext &, JsonValue &, std::u8string_view, JsonType);
int json_parse_number(JsonContext &, JsonValue &);
int json_parse_string(JsonContext &, JsonValue &);
int json_parse_array(JsonContext &, JsonValue &);
std::u8string json_encode_utf8(unsigned);

