#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>
#include "json.hpp"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)                                                           \
    do                                                                                                             \
    {                                                                                                              \
        test_count++;                                                                                              \
        if (equality)                                                                                              \
            test_pass++;                                                                                           \
        else                                                                                                       \
        {                                                                                                          \
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual); \
            main_ret = 1;                                                                                          \
        }                                                                                                          \
    } while (0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
/*
inline void EXPECT_EQ_INT(int expect, int actual) {
    printf("Testing %d == %d\n", expect, actual);
    EXPECT_EQ_BASE(expect == actual, expect, actual, "%d");
}
*/
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%lf")
#define EXPECT_EQ_STRING(expect, actual) EXPECT_EQ_BASE((expect) == (actual), std::string(expect.begin(), expect.end()).c_str(), std::string(actual.begin(), actual.end()).c_str(), "%s")
#define TEXT(quote) (u8"\"" quote u8"\"")

#define TEST_NUMBER(expect, json)                     \
    do                                                \
    {                                                 \
        JsonValue v;                                  \
        EXPECT_EQ_INT(PARSE_OK, json_parse(v, json)); \
        EXPECT_EQ_INT(JSON_NUMBER, v.get_type()); \
        EXPECT_EQ_DOUBLE(expect, v.get_number()); \
    } while (0)


std::string VALUE_TO_STRING(JsonValue &v);
std::string ARRAY_TO_STRING(std::vector<JsonValue>& array)
{
    std::string temp;
    temp.push_back('[');
    for(auto i = array.begin(); i != array.end(); i++)
    {
        temp += VALUE_TO_STRING(*i);
        temp += ", ";
    }
    temp = temp.substr(0, temp.length() - 2);
    temp.push_back(']');
    return temp;
}

std::string OBJECT_TO_STRING(std::map<std::u8string, JsonValue> object)
{
    std::string r;
    r += "{";
    for (auto &i : object)
    {
        r += "\"" + std::string(i.first.begin(), i.first.end()) + "\"";
        r += " : ";
        r += VALUE_TO_STRING(i.second);
        r += ", ";
    }
    r = r.substr(0, r.length() - 2);
    r += "}";
    return r;
}
std::string VALUE_TO_STRING(JsonValue &v)
{
    switch (v.get_type())
    {
    case JSON_NULL:
        return "null";
    case JSON_FALSE:
        return "false";
    case JSON_TRUE:
        return "true";
    case JSON_NUMBER:
        return std::to_string(v.get_number());
        break;
    case JSON_STRING:
        return '\"' + std::string(v.get_string().begin(), v.get_string().end()) + '\"';
    case JSON_ARRAY:
        return ARRAY_TO_STRING(v.get_array());
    case JSON_OBJECT:
        return OBJECT_TO_STRING(v.get_object());
    }
}

inline void EXPECT_EQ_ARRAY(std::vector<JsonValue> &expect, std::vector<JsonValue> &actual)
{
    std::string expect_str(ARRAY_TO_STRING(expect)), actual_str(ARRAY_TO_STRING(actual));
    EXPECT_EQ_BASE((expect) == (actual), expect_str.c_str(), actual_str.c_str(), "%s");
}

inline void EXPECT_EQ_OBJECT(std::map<std::u8string, JsonValue> &expect,
                             std::map<std::u8string, JsonValue> &actual)
{
    std::string expect_str(OBJECT_TO_STRING(expect)), actual_str(OBJECT_TO_STRING(actual));
    EXPECT_EQ_BASE((expect) == (actual), expect_str.c_str(), actual_str.c_str(), "%s");
}

inline void TEST_STRING(std::u8string_view expect, std::u8string_view json)
{
    JsonValue v;
    EXPECT_EQ_INT(PARSE_OK, json_parse(v, json));
    EXPECT_EQ_INT(JSON_STRING, v.get_type());
    EXPECT_EQ_STRING(std::u8string(expect), v.get_string());
}
inline void TEST_ARRAY(std::vector<JsonValue> expect, std::u8string_view json)
{
    JsonValue v;
    EXPECT_EQ_INT(PARSE_OK, json_parse(v, json));
    EXPECT_EQ_INT(JSON_ARRAY, v.get_type());
    EXPECT_EQ_ARRAY(expect, v.get_array());
}

inline void TEST_OBJECT(std::map<std::u8string, JsonValue> expect,
                        std::u8string_view json)
{
    JsonValue v;
    EXPECT_EQ_INT(PARSE_OK, json_parse(v, json));
    EXPECT_EQ_INT(JSON_OBJECT, v.get_type());
    EXPECT_EQ_OBJECT(expect, v.get_object());
}

#define TEST_ERROR(error, json)                    \
    do                                             \
    {                                              \
        JsonValue v;                               \
        v.set_type(JSON_FALSE);                    \
        EXPECT_EQ_INT(error, json_parse(v, json)); \
        EXPECT_EQ_INT(JSON_NULL, v.get_type());    \
    } while (0)

static void test_parse_error(){
    TEST_ERROR(PARSE_EXPECT_VALUE, u8"");
    TEST_ERROR(PARSE_EXPECT_VALUE, u8" ");
    TEST_ERROR(PARSE_INVALID_VALUE, u8"tru");
    TEST_ERROR(PARSE_INVALID_VALUE, u8"fal");
    TEST_ERROR(PARSE_INVALID_VALUE, u8"nul");
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, u8"null a");
    /* invalid number */
    TEST_ERROR(PARSE_INVALID_VALUE, u8"+0");
    TEST_ERROR(PARSE_INVALID_VALUE, u8"+1");
    TEST_ERROR(PARSE_INVALID_VALUE, u8".123"); /* at least one digit before '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, u8"1.");   /* at least one digit after '.' */
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, u8"1.1.23");   /* only one '.' */
    TEST_ERROR(PARSE_INVALID_VALUE, u8"INF");
    TEST_ERROR(PARSE_INVALID_VALUE, u8"inf");
    TEST_ERROR(PARSE_INVALID_VALUE, u8"NAN");
    TEST_ERROR(PARSE_INVALID_VALUE, u8"nan");
    /* invalid string */
    TEST_ERROR(PARSE_INVALID_STRING_END, u8"\"Text");
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, TEXT(u8"\t"));
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, u8"\"Text\" null" );
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, TEXT(u8"\\q"));
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, TEXT(u8"\\u01Gh"));
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, TEXT(u8"\\ud83d|ude00"));
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, TEXT(u8"\\ud83d\\nde00"));
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, TEXT(u8"\\ud83d\\n0000"));
    /* invalid array */
    TEST_ERROR(PARSE_INVAID_ARRAY_END, u8"[1, 2");
    TEST_ERROR(PARSE_EXTRA_ARRAY_SEPARATOR, u8"[1, 2,]");
    /* invalid object*/
    TEST_ERROR(PARSE_INVAID_OBJECT_END,
               u8"{\"Key\": null");
    TEST_ERROR(PARSE_INVALID_OBJECT_KEY,
               u8"{\"Key: null}");
    TEST_ERROR(PARSE_INVALID_OBJECT_SEPARATOR,
               u8"{\"Key\"| null}");
    TEST_ERROR(PARSE_INVALID_OBJECT_VALUE,
               u8"{\"Key\": nul}");
    TEST_ERROR(PARSE_EXTRA_OBJECT_SEPARATOR,
               u8"{\"Key\": null,}");
}

static void test_parse_null() {
    JsonValue v;
    v.set_type(JSON_TRUE);
    EXPECT_EQ_INT(PARSE_OK, json_parse(v, u8"null"));
    EXPECT_EQ_INT(JSON_NULL, v.get_type());
}

static void test_parse_true() {
    JsonValue v;
    v.set_type(JSON_FALSE);
    EXPECT_EQ_INT(PARSE_OK, json_parse(v, u8"true"));
    EXPECT_EQ_INT(JSON_TRUE, v.get_type());
}

static void test_parse_false() {
    JsonValue v;
    v.set_type(JSON_TRUE);
    EXPECT_EQ_INT(PARSE_OK, json_parse(v, u8"false"));
    EXPECT_EQ_INT(JSON_FALSE, v.get_type());
}

static void test_parse_number() {
    TEST_NUMBER(0.0, u8"0");
    TEST_NUMBER(0.0, u8"-0");
    TEST_NUMBER(0.0, u8"-0.0");
    TEST_NUMBER(1.0, u8"1");
    TEST_NUMBER(-1.0, u8"-1");
    TEST_NUMBER(1.5, u8"1.5");
    TEST_NUMBER(-1.5, u8"-1.5");
    TEST_NUMBER(3.1416, u8"3.1416");
    TEST_NUMBER(1E10, u8"1E10");
    TEST_NUMBER(1e10, u8"1e10");
    TEST_NUMBER(1E+10, u8"1E+10");
    TEST_NUMBER(1E-10, u8"1E-10");
    TEST_NUMBER(-1E10, u8"-1E10");
    TEST_NUMBER(-1e10, u8"-1e10");
    TEST_NUMBER(-1E+10, u8"-1E+10");
    TEST_NUMBER(-1E-10, u8"-1E-10");
    TEST_NUMBER(1.234E+10, u8"1.234E+10");
    TEST_NUMBER(1.234E-10, u8"1.234E-10");
}

static void test_parse_string() {
    //Use TEXT() marco add "" automatically
    TEST_STRING(u8"", TEXT(u8""));
    TEST_STRING(u8"Text", TEXT(u8"Text"));
    TEST_STRING(u8"Text", u8"\"Text\"  ");       //whitespace after string
    TEST_STRING(u8"\"\"", TEXT(u8"\\\"\\\""));   //\"\" -> ""
    TEST_STRING(u8"/", TEXT(u8"\\/"));
    TEST_STRING(u8"\b", TEXT(u8"\\b"));          
    TEST_STRING(u8"\f", TEXT(u8"\\f"));          
    TEST_STRING(u8"\n", TEXT(u8"\\n"));          
    TEST_STRING(u8"\t", TEXT(u8"\\t"));          
    TEST_STRING(u8"\r", TEXT(u8"\\r"));
    //test parsing unicode
    TEST_STRING(u8"中文", TEXT(u8"\\u4e2d\\u6587"));
    TEST_STRING(u8"€", TEXT(u8"\\u20ac"));
    TEST_STRING(u8"😀",TEXT(u8"\\ud83d\\ude00"));
    using namespace std::literals;
    TEST_STRING(u8"中\0文"sv, TEXT(u8"\\u4e2d\\u0000\\u6587")); //include '\0'
}

static void test_parse_array() {
    TEST_ARRAY({}, u8"[]");
    TEST_ARRAY({JSON_NULL, JSON_FALSE, JSON_TRUE, 1.0, u8"Text"}, u8"[null, false, true, 1.0, \"Text\"]");
    TEST_ARRAY({std::vector<JsonValue>{}, std::vector<JsonValue>{0.0}, std::vector<JsonValue>{0.0, 1.0}},
               u8"[ [ ] , [ 0 ] , [ 0 , 1 ] ]");
}

static void test_parse_object() {
    std::vector<JsonValue> test_array = {JSON_NULL, 1.23, u8"Text"};
    std::map<std::u8string, JsonValue> test_object{{u8"Object1", test_array}};
    std::map<std::u8string, JsonValue> expect {
        {u8"Null", JSON_NULL},
        {u8"False", JSON_FALSE},
        {u8"True", JSON_TRUE},
        {u8"Text", u8"This is a text."},
        {u8"Array", test_array},
        {u8"Object", test_object}
    };
    std::u8string_view json =
        u8"{"
        u8"  \"Null\": null,"
        u8"  \"False\": false,"
        u8"  \"True\": true,"
        u8"  \"Text\": \"This is a text.\","
        u8"  \"Array\": [null, 1.23, \"Text\"],"
        u8"  \"Object\": { \"Object1\" : [null, 1.23, \"Text\"] }"
        u8"}";
    TEST_OBJECT(expect, json);
}

static void test_parse() {
    test_parse_error();
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}