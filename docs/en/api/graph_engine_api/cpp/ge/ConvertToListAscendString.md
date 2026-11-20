# ConvertToListAscendString

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:16:27.517Z pushedAt=2026-10-28T07:40:22.142Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Defines a template function **ConvertToListAscendString** for converting string lists of different types into a list of the **AscendString** type.

## Prototype

```c++
template<typename T> std::vector<ge::AscendString> ConvertToListAscendString(T strs)
```

The following two specializations are supported:

- template<\> inline std::vector\<ge::AscendString\> ConvertToListAscendString\(std::vector\<std::string\> strs\)

    For a string list of the `std::vector<std::string>` type, it is first converted to the std::vector\<const char \*\> type and then converted.

- template<\> inline std::vector\<ge::AscendString\> ConvertToListAscendString\(std::vector\<ge::AscendString\> strs\)

    For a string list of the std::vector\<ge::AscendString\> type, the original list is returned directly.

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| strs | Input | String list to be converted. |

## Return Value

Returns the converted string list of the **AscendString** type.

## Constraints

None
