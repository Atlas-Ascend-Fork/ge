# ConvertToAscendString

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:15:18.360Z pushedAt=2026-10-28T07:40:22.141Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

A template function that accepts a template parameter **T** and converts it to the AscendString type. The main purpose of this function is to convert strings of different types to the AscendString type.

## Prototype

```c++
template<typename T> ge::AscendString ConvertToAscendString(T str)
```

The following overloads are supported:

- template<\> inline ge::AscendString ConvertToAscendString\<const char \*\>\(const char \*str\)

    For a string of the const char \* type, the AscendString constructor is used directly for conversion.

- template<\> inline ge::AscendString ConvertToAscendString\<std::string\>\(std::string str\)

    For a string of the std::string type, it is first converted to the const char \* type and then converted.

- template<\> inline ge::AscendString ConvertToAscendString\<ge::AscendString\>\(ge::AscendString str\)

    For a string of the AscendString type, the AscendString type string is returned directly.

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| str | Input | String to be converted. |

## Return Value

Returns the converted **AscendString** string.

## Constraints

None
