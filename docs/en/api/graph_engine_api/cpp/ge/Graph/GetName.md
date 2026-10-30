# GetName

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:18:35.451Z pushedAt=2026-10-28T07:40:22.748Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Obtains the name of the current graph.

## Prototype

> [!NOTE]
> The API whose data type is **string** will be deprecated in later versions. It is recommended to use the API whose data type is not **string**.

```c++
const std::string &Graph::GetName() const
graphStatus GetName(AscendString &name) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Output | Name of the graph to be obtained. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **SUCCESS**: The graph name is obtained successfully. |

## Constraints

None
