# GetOpsTypeList

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:34:52.432Z pushedAt=2026-10-28T07:42:19.723Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/operator\_factory.h\>
- Library file: libgraph.so

## Description

Obtains the list of all operator types supported by the system.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. You are advised to use the API with a non-string data type.

```c++
static graphStatus GetOpsTypeList(std::vector<std::string> &all_ops)
static graphStatus GetOpsTypeList(std::vector<AscendString> &all_ops)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| all_ops | Output | List of operator types. |

## Return Value

graphStatus type: GRAPH\_SUCCESS, indicating success; GRAPH\_FAILED, indicating failure.

## Constraints

None
