# GetDynamicSubgraph

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:18:49.463Z pushedAt=2026-10-28T07:42:19.668Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the dynamic input subgraph corresponding to an operator based on the subgraph name and subgraph index.

## Prototype

> [!NOTE]
> APIs with string data type will be deprecated in future versions. It is recommended to use APIs with non-string data types.

```c++
Graph GetDynamicSubgraph(const std::string &name, uint32_t index) const
Graph GetDynamicSubgraph(const char_t *name, uint32_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Subgraph name. |
| index | Input | Index of the subgraph with the same name. |

## Return Value

Graph object.

## Constraints

None
