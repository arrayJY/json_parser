#include "json.hpp"
#include <cassert>
#include <charconv>
#include <string>
#include <string_view>

int json_parse(JsonValue &v, std::u8string_view json)
{
    JsonContext c;
    c.json = json;

    v.set_type(JSON_NULL);
    json_parse_whitespace(c);
    int ret;
    if((ret = json_parse_value(c, v)) == PARSE_OK)
    {
        json_parse_whitespace(c);
        if(!c.json.empty())
        {
            v.set_type(JSON_NULL);
            ret = PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

void json_parse_whitespace(JsonContext &context)
{
    int i = 0;
    const std::u8string_view &json = context.json;
    while(json[i] == ' ' || json[i] ==  '\t' || json[i] == '\n' || json[i] == '\r')
        i++;
    context.json = json.substr(i);
}

int json_parse_value(JsonContext& context, JsonValue &value)
{
    switch(*context.json.begin())
    {
        case u8'n':
            return json_parse_literal(context, value, u8"null", JSON_NULL);
        case u8'f':
            return json_parse_literal(context, value, u8"false", JSON_FALSE);
        case u8't':
            return json_parse_literal(context, value, u8"true", JSON_TRUE);
        case u8'\"':
            return json_parse_string(context, value);
        case u8'[':
            return json_parse_array(context, value);
        case u8'{':
            return json_parse_object(context, value);
        case u8'\0':
            return PARSE_EXPECT_VALUE;
        default:
            return json_parse_number(context, value);
    }

}


int json_parse_literal(JsonContext &c, JsonValue &v, const std::u8string_view literal, JsonType type)
{
    if(!c.json.starts_with(literal))
        return PARSE_INVALID_VALUE;
    v.set_type(type);
    c.json = c.json.substr(literal.length());
    return PARSE_OK;
}

int json_parse_number(JsonContext &c, JsonValue &v)
{
    //validate number
    size_t number_length = 0;
    {
        auto is_digital = [](const char ch) { return ch >= u8'0' && ch <= u8'9'; };
        auto is_digital_1to9 = [](const char ch) { return ch >= u8'1' && ch <= u8'9'; };
        auto current = c.json.begin();
        /* negative part */
        if(*current == u8'-')
            ++current;
        
        /* integer part */
        if(is_digital_1to9(*current))
        {
            while(is_digital(*current))
                ++current;
        }
        else if(*current == u8'0')
        {
            ++current;
        }
        else
            return PARSE_INVALID_VALUE;

        /* decimal part */
        if(*current == '.')
        {
            ++current;
            //Must have one digital after .
            if(!is_digital(*current))
                return PARSE_INVALID_VALUE;

            while (is_digital(*current))
                ++current;
        }
        /* index part */
        if(*current == u8'e' || *current == u8'E')
        {
            ++current;
            if(*current == u8'-' || *current == u8'+')
                ++current;
            while(is_digital(*current))
                ++current;
        }
        /* validate end */
        /*
        if(!is_whitespace_or_end(*current))
            return PARSE_INVALID_VALUE;
            */

        number_length = current - c.json.begin();
    }
    v.set_number(std::stod(std::string(c.json.begin(), c.json.begin() + number_length)));
    v.set_type(JSON_NUMBER);
    c.json = c.json.substr(number_length);
    return PARSE_OK;
}

int json_parse_string_raw(JsonContext &c, std::u8string &str, size_t& end_pos)
{
    for (auto i = c.json.begin() + 1; i != c.json.end(); i++)
    {
        //Another quotation
        if(*i == '\"' && *(i-1) != '\\')
        {
            end_pos = i - c.json.begin() + 1;
            break;
        }

        if(static_cast<unsigned char>(*i) < 0x20)
            return PARSE_INVALID_STRING_CHAR;

        if(*i == u8'\\')
        //Todo: parse Unicode
        {
            switch(*(++i))
            {
            case u8'\\':
                str.push_back(u8'\\');
                break;
            case u8'\"':
                str.push_back(u8'\"');
                break;
            case u8'/':
                str.push_back(u8'/');
                break;
            case u8'b':
                str.push_back(u8'\b');
                break;
            case u8'f':
                str.push_back(u8'\f');
                break;
            case u8'n':
                str.push_back(u8'\n');
                break;
            case u8't':
                str.push_back(u8'\t');
                break;
            case u8'r':
                str.push_back(u8'\r');
                break;
            case u8'u':
            {
                auto check_codepoint_vaild = [&c](auto i) {
                    for (auto m = i; m < i + 4; m++)
                    {
                        if(m == c.json.end() || 
                           !((*m >= u8'0' && *m <= u8'9') ||
                             (*m >= u8'a' && *m <= u8'f') || 
                             (*m >= u8'A' && *m <= u8'F')))
                            return false;
                    }
                    return true;
                };
                ++i;
                if(!check_codepoint_vaild(i))
                    return PARSE_INVALID_UNICODE_HEX;
                unsigned codepoint = std::stoul(std::string(i, i + 4), nullptr, 16);
                /*
                * Json use surrogate pair to represente U+10000~U+10FFFF,
                * which like \uXXXX\uYYYY 
                * H = XXXX = U+D800 ~ U+DBFF
                * L = YYYY = U+DC00 ~ U+DFFF
                * codepoint = 0x10000 + (H − 0xD800) * 0x400 + (L − 0xDC00)
                */
                if(codepoint >= 0xD800 && codepoint <= 0xDBFF)
                {
                    i += 4; //Next codepoint
                    if(*i++ != u8'\\')
                        return PARSE_INVALID_UNICODE_SURROGATE;
                    if(*i++ != u8'u')
                        return PARSE_INVALID_UNICODE_SURROGATE;

                    unsigned high_surrogate = codepoint;
                    if(!check_codepoint_vaild(i))
                        return PARSE_INVALID_UNICODE_HEX;
                    unsigned low_surrogate = std::stoul(std::string(i, i + 4), nullptr, 16);
                    if(low_surrogate < 0xDC00 || low_surrogate > 0xDFFF)
                        return PARSE_INVALID_UNICODE_SURROGATE;

                    codepoint = 0x10000 + (high_surrogate - 0xD800) * 0x400 + (low_surrogate - 0xDC00);
                }
                str += json_encode_utf8(codepoint);
                i += 3;
                break;
            }
            default:
                return PARSE_INVALID_STRING_ESCAPE;
            }
            continue;
        }
        else
            str.push_back(*i);
    }
    if(!end_pos)
        return PARSE_INVALID_STRING_END;
    return PARSE_OK;
}

int json_parse_string(JsonContext &c, JsonValue &v)
{
    int ret = PARSE_OK;
    size_t end_quotation_pos = 0;
    std::u8string temp;
    if((ret = json_parse_string_raw(c, temp, end_quotation_pos)) != PARSE_OK)
        return ret;
    v.set_type(JSON_STRING);
    v.set_string(temp);
    c.json = c.json.substr(end_quotation_pos);
    return PARSE_OK;
}

std::u8string json_encode_utf8(unsigned codepoint)
{
    assert(codepoint <= 0x10FFFF);
    std::u8string temp;
    //U+0000~U+007F -> 1 byte: 0xxx xxxx
    if(codepoint <= 0x007F)
    {
        temp.push_back(codepoint & 0x7F);                   //0x7F = 0111 1111
    }
    //U+0080~U+07FF-> 2 bytes: 110x xxxx, 10yy yyyy
    else if (0x0080 <= codepoint && codepoint <= 0x07FF)
    {
        temp.push_back(0xC0 | ((codepoint >> 6) & 0x1F));   //0xC0 = 1100 0000, 0x1F = 0001 1111
        temp.push_back(0x80 | ( codepoint       & 0x3F));   //0x80 = 1000 0000, 0x3F = 0011 1111
    }
    //U+0800~U+FFFF-> 3 bytes: 1110 xxxx, 10yy yyyy, 10zz zzzz
    else if (0x0080 <= codepoint && codepoint <= 0xFFFF)
    {
        temp.push_back(0xE0 | ((codepoint >> 12) & 0x0F));   //0xE0 = 1110 0000, 0x0F = 0000 1111
        temp.push_back(0x80 | ((codepoint >>  6) & 0x3F));   //0x80 = 1000 0000, 0x3F = 0011 1111
        temp.push_back(0x80 | ( codepoint        & 0x3F));   //0x80 = 1000 0000, 0x3F = 0011 1111
    }
    else
    //U+10000~U+10FFFFF-> 4 bytes: 1111 0xxx, 10yy yyyy, 10zz zzzz, 10mm mmmm
    {
        temp.push_back(0xF0 | ((codepoint >> 18) & 0x07));   //0xF0 = 1111 0000, 0x07 = 0000 0111
        temp.push_back(0x80 | ((codepoint >> 12) & 0x3F));   //0x80 = 1000 0000, 0x3F = 0011 1111
        temp.push_back(0x80 | ((codepoint >>  6) & 0x3F));   //0x80 = 1000 0000, 0x3F = 0011 1111
        temp.push_back(0x80 | ( codepoint        & 0x3F));   //0x80 = 1000 0000, 0x3F = 0011 1111
    }
    return temp;
}

int json_parse_array(JsonContext &c, JsonValue &v)
{
    c.json = c.json.substr(1);
    std::vector<JsonValue> result;
    int ret = PARSE_OK;
    json_parse_whitespace(c);
    while(ret == PARSE_OK && !c.json.starts_with(u8"]"))
    {
        if(c.json.length() == 0)
            return PARSE_INVAID_ARRAY_END;
        JsonValue value;
        ret = json_parse_value(c, value);
        result.push_back(value);
        json_parse_whitespace(c);

        //handle ','
        if(c.json.starts_with(u8","))
        {
            c.json = c.json.substr(1);
            json_parse_whitespace(c);
            if(c.json.starts_with(u8"]"))
                return PARSE_EXTRA_ARRAY_SEPARATOR;
        }
    }
    //handle ']'
    c.json = c.json.substr(1);
    if (ret == PARSE_OK)
    {
        v.set_type(JSON_ARRAY);
        v.set_array(result);
    }
    return ret;
}

int json_parse_object(JsonContext &c, JsonValue &v)
{
    //handle '{'
    c.json = c.json.substr(1);
    std::map<std::u8string, JsonValue> result;
    int ret = PARSE_OK;
    json_parse_whitespace(c);
    while(ret == PARSE_OK && !c.json.starts_with(u8"}"))
    {
        if(c.json.empty())
            return PARSE_INVAID_OBJECT_END;

        std::u8string key;
        size_t key_end_pos = 0;
        if((ret = json_parse_string_raw(c, key, key_end_pos)) != PARSE_OK)
            return PARSE_INVALID_OBJECT_KEY;
        c.json = c.json.substr(key_end_pos);

        json_parse_whitespace(c);
        if(!c.json.starts_with(u8":"))
            return PARSE_INVALID_OBJECT_SEPARATOR;
        //handle ':'
        c.json = c.json.substr(1);
        json_parse_whitespace(c);

        JsonValue value;
        if((ret = json_parse_value(c, value)) != PARSE_OK)
            return PARSE_INVALID_OBJECT_VALUE;

        result.insert({key, value});

        json_parse_whitespace(c);
        //handle ','
        if(c.json.starts_with(u8","))
        {
            c.json = c.json.substr(1);
            json_parse_whitespace(c);
            if(c.json.starts_with(u8"}"))
                return PARSE_EXTRA_OBJECT_SEPARATOR;
        }
    }
    //handle '}'
    c.json = c.json.substr(1);
    if (ret == PARSE_OK)
    {
        v.set_type(JSON_OBJECT);
        v.set_object(result);
    }
    return ret;
}

JsonType JsonValue::get_type()
{
    return type;
}

double JsonValue::get_number()
{
    assert(type == JSON_NUMBER);
    return number;
}

std::u8string& JsonValue::get_string()
{
    assert(type == JSON_STRING);
    return text;
}

std::vector<JsonValue> &JsonValue::get_array()
{
    assert(type == JSON_ARRAY);
    return array;
}

std::map<std::u8string, JsonValue> &JsonValue::get_object()
{
    assert(type == JSON_OBJECT);
    return object;
}

JsonValue &JsonValue::operator=(const double v){
    type = JSON_NUMBER;
    number = v;

    text.clear();
    array.clear();
    object.clear();
    return *this;
}
JsonValue &JsonValue::operator=(const JsonType t) {
    type = t;

    number = 0.0;
    text.clear();
    array.clear();
    object.clear();
    return *this;
}

JsonValue &JsonValue::operator=(const std::u8string_view str)
{
    type = JSON_STRING;
    text = str;

    number = 0.0;
    array.clear();
    object.clear();
    return *this;
}
JsonValue &JsonValue::operator=(const std::vector<JsonValue> &a)
{
    type = JSON_ARRAY;
    array = a;

    number = 0.0;
    text.clear();
    object.clear();
    return *this;
}
JsonValue &JsonValue::operator=(const std::map<std::u8string, JsonValue> &o)
{
    type = JSON_OBJECT;
    object = o;

    number = 0.0;
    text.clear();
    array.clear();
    return *this;
}

int inline decode_utf8(int *state, int *codep, int byte);
std::u8string codepoint_to_string(int codepoint);
std::u8string JsonValue::to_string()
{
    std::u8string str;
    switch (type)
    {
    case JSON_NULL:
        return u8"null";
    case JSON_FALSE:
        return u8"false";
    case JSON_TRUE:
        return u8"true";
    case JSON_NUMBER:
    {
        std::string temp = std::to_string(number);
        return std::u8string(temp.begin(), temp.end());
    }
    case JSON_STRING:
    {
        str.push_back(u8'\"');
        int codepoint = 0, state = 0;
        for (auto s = text.begin(); s != text.end(); ++s)
        {
            if(decode_utf8(&state, &codepoint, *s))
                continue;
            switch(codepoint)
            {
            case u8'\\':
                str += u8"\\\\";
                break;
            case u8'/':
                str += u8"\\/";
                break;
            case u8'\"':
                str += u8"\\\"";
                break;
            case u8'\n':
                str += u8"\\n";
                break;
            case u8'\b':
                str += u8"\\b";
                break;
            case u8'\f':
                str += u8"\\f";
                break;
            case u8'\t':
                str += u8"\\t";
                break;
            case u8'\r':
                str += u8"\\r";
                break;
            default:
                if(codepoint > 0x20 && codepoint < 0x7F)
                    str.push_back(static_cast<char8_t>(codepoint));
                else
                    str += codepoint_to_string(codepoint);
            }
        }
        str.push_back(u8'\"');
        break;
    }
    case JSON_ARRAY:
        str.push_back(u8'[');
        for(auto &i: array)
        {
            str += i.to_string();
            str.push_back(u8',');
        }
        str.pop_back();
        str.push_back(u8']');
        break;
    case JSON_OBJECT:
        str.push_back(u8'{');
        for(auto &i: object)
        {
            str += u8"\"" + i.first + u8"\"";
            str.push_back(u8':');
            str += i.second.to_string();
            str.push_back(u8',');
        }
        str.pop_back();
        str.push_back(u8'}');
        break;
    }
    return str;
}

void JsonValue::set_number(double n) { number = n; }
void JsonValue::set_type(JsonType t) { type = t; }
void JsonValue::set_string(const std::u8string& str) { text = str; }
void JsonValue::set_array(const std::vector<JsonValue>& v) { array = v; }
void JsonValue::set_object(const std::map<std::u8string, JsonValue> &o) { object = o; }

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
int inline decode_utf8(int *state, int *codep, int byte)
{
    const int UTF8_ACCEPT = 0;
    static const char8_t utf8d[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
    0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
    0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
    0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
    1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
    1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
    1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
    };

    int type = utf8d[byte];

    *codep = (*state != UTF8_ACCEPT) ?
        (byte & 0x3fu) | (*codep << 6) :
        (0xff >> type) & (byte);

    *state = utf8d[256 + *state*16 + type];
    return *state;
}
std::u8string codepoint_to_string(int codepoint)
{
    std::u8string next;
    char str[4];
    if(codepoint > 0xFFFF)
    {
        int h = 0xD800 + ((codepoint - 0x10000) >> 10);
        int l = 0xDC00 + ((codepoint - 0x10000) & 0x3FF);
        std::to_chars(str, str + 4, l, 16);
        next = u8"\\u" + std::u8string(str, str + 4);
        codepoint = h;
    }
    std::to_chars(str, str + 4, codepoint, 16);
    return u8"\\u" + std::u8string(str, str + 4) + next; 
}