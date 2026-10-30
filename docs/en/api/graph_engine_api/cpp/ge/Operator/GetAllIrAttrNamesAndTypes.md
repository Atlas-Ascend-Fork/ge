# GetAllIrAttrNamesAndTypes

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:15:53.451Z pushedAt=2026-10-28T07:42:19.659Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the names and types of all attributes defined in the IR prototype of the operator, including both common and required attributes.

## Prototype

```c++
graphStatus GetAllIrAttrNamesAndTypes(std::map<AscendString, AscendString> &attr_name_types) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| attr_name_types | Output | Names and types of all attributes defined in the IR prototype. |

## Return Value

**graphStatus** type: **GRAPH\_SUCCESS**, indicating success; **GRAPH\_FAILED**, indicating failure.

## Constraints

None
