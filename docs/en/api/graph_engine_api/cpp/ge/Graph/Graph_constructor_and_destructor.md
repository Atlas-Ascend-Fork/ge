# Graph Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:19:22.944Z pushedAt=2026-10-28T07:40:22.752Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/graph.h\>
- Library file: libgraph.so

## Description

Graph constructor and destructor.

## Prototype

> [!NOTE]
> The APIs whose data type is string will be deprecated in later versions. It is recommended to use the APIs whose data type is not string and that pass the name parameter. The default parameterless constructor constructs an invalid Graph object.

```c++
explicit Graph(const std::string& name)
explicit Graph(const char *name)
Graph()
~Graph()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Graph name. Constructs a Graph with the specified name. |

## Return Value

The Graph constructor returns an object of the Graph type.

## Constraints

None
