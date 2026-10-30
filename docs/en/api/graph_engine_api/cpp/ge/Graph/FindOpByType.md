# FindOpByType

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:16:17.079Z pushedAt=2026-10-28T07:40:22.740Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains all cached operator objects of the specified type from the Graph based on the operator type.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. It is recommended to use the API with a non-string data type.

```c++
graphStatus FindOpByType(const std::string &type, std::vector<ge::Operator> &ops) const
graphStatus FindOpByType(const char_t *type, std::vector<ge::Operator> &ops) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| type | Input | Operator type to be obtained. |
| ops | Output | Operator objects required by the user. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **GRAPH_SUCCESS(0)**: Success.<br>Other values: Failure. |

## Constraints

None
