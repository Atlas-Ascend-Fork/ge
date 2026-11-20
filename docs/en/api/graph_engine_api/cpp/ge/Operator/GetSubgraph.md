# GetSubgraph

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:23:38.956Z pushedAt=2026-10-28T07:42:19.685Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the subgraph corresponding to the operator based on the subgraph name.

## Prototype

> [!NOTE]
> APIs with the string data type will be deprecated in future versions. You are advised to use APIs with non-string data types.

```c++
Graph GetSubgraph(const std::string &name) const
Graph GetSubgraph(const char_t *name) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Subgraph name. |

## Return Value

Graph object.

## Constraints

None
