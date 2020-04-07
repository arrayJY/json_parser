# JSON Parser

![](https://img.shields.io/badge/build-pass-green?style=flat-square) ![](https://img.shields.io/badge/standard-C++20-red?style=flat-square) ![](https://img.shields.io/badge/Powered_By-arrayJY-yellow?style=flat-square)  

JSON Parser is an experimental tools writing in C++20.

It's slow, useless and filled with bugs.

 Just for FUN😁.



Thanks to the [tutorial](https://zhuanlan.zhihu.com/json-tutorial) by [Milo Yip](https://github.com/miloyip).

## Compile

The parser use `std::u8string` of C++20. So the compiler must support C++20, such as Clang-10 or GCC-9.

Use the command with `-std=c++2a` like:

```
$ clang++-10 test.cpp -o json -std=c++2a
```

## Usage

First, include the header file:

```cpp
#include "json.hpp"
using namespace Json;
```

Use `parse_json()` to generate `JsonValue`, it will return `PARSE_OK` when parse successfully.

```cpp
std::u8string json;		//std::u8string is only suppored in C++20 now.
/* ...Read json text... */
JsonValue root;
int success = parse_json(root, json);
if(sucess == PARSE_OK)
	/*...*/
```

`JsonValue` has 7 possible `JsonType`, using `JsonValue::get_type()` to get it:

* `JSON_NULL`
* `JSON_FALSE`
* `JSON_TRUE`
* `JSON_NUMBER`
* `JSON_STRING`
* `JSON_ARRAY`
* `JSON_OBJECT`

It will handle types automatically when you try to get value, you must ensure calling correct `get_methods()`.

```cpp
assert(root.get_type() == JSON_OBJECT);
auto o = root.get_object();

auto v = root["Value"];
assert(v.get_type() == JSON_NUMBER);
double v = v.get_number();
```

But you can construct or assign `JsonValue` without concerning types.

```cpp
JsonValue v(JSON_TRUE);
JsonType t1 = v.get_type(); //JSON_TRUE
JsonValue v = 1.0;
JsonType t2 = v.get_type(); //JSON_NUMBER
```

You can call `JsonValue::to_string()` for serialization. It will return a `std::u8string`.

```cpp
JsonValue v(std::vector<JsonValue>{JSON_NULL, u8"Text"});
auto str = v.to_string()   //u8"[null,\"Text\"]"
```



## License

[MIT © arrayJY](https://github.com/arrayJY/json_parser/blob/master/LICENSE)

