# GetDynamicSubgraphBuilder

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:19:09.459Z pushedAt=2026-10-28T07:42:19.670Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the builder function object of the dynamic input subgraph corresponding to the operator based on the subgraph name and subgraph index.

## Prototype

> [!NOTE]
> The APIs with the string data type will be deprecated in later versions. You are advised to use the APIs with a non-string data type.

```c++
SubgraphBuilder GetDynamicSubgraphBuilder(const std::string &name, uint32_t index) const
SubgraphBuilder GetDynamicSubgraphBuilder(const char_t *name, uint32_t index) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Subgraph name. |
| index | Input | Index of the subgraphs with the same name. |

## Return Value

SubgraphBuilder object.

## Constraints

None
