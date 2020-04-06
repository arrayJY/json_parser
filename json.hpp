#pragma once
#include <string>
#include <string_view>

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
    JsonValue() : number(0.0), type(JSON_NULL), text(u8"") {}
    JsonType get_type();
    double get_number();
    const std::u8string& get_string();

    void set_type(JsonType);
    void set_number(double);
    void set_string(const std::u8string&);

private:
    double number;
    JsonType type;
    std::u8string text;
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
};

int json_parse(JsonValue &, std::u8string_view);
void json_parse_whitespace(JsonContext&);
int json_parse_value(JsonContext&, JsonValue&);
int json_parse_literal(JsonContext &, JsonValue &, std::u8string_view, JsonType);
int json_parse_number(JsonContext &, JsonValue &);
int json_parse_string(JsonContext &, JsonValue &);
std::u8string json_encode_utf8(unsigned);

