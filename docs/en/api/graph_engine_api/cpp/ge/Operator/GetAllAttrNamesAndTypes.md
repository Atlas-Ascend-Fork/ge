# GetAllAttrNamesAndTypes

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:15:56.636Z pushedAt=2026-10-28T07:42:19.660Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the names and types of all attributes that have been set for the operator, including common attributes defined in the IR prototype and user-defined attributes.

## Prototype

> [!NOTE]
> The API whose data type is string will be deprecated in later versions. Use the API whose data type is not string instead.

```c++
const std::map<std::string, std::string> GetAllAttrNamesAndTypes() const
graphStatus GetAllAttrNamesAndTypes(std::map<AscendString, AscendString> &attr_name_types) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| attr_name_types | Output | Names and types of all attributes. |

## Return Value

**graphStatus** type: **GRAPH\_SUCCESS**, indicating success; **GRAPH\_FAILED**, indicating failure.

## Constraints

None
