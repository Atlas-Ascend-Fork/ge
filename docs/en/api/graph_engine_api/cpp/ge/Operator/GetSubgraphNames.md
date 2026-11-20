# GetSubgraphNames

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:24:37.286Z pushedAt=2026-10-28T07:42:19.687Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the list of subgraph names in the operator IR information.

## Prototype

> [!NOTE]
> The API whose data type is string will be deprecated in later versions. Use the API whose data type is not string instead.

```c++
std::vector<std::string> GetSubgraphNames() const
graphStatus GetSubgraphNames(std::vector<AscendString> &names) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| names | Output | Obtains the list of subgraph names of an operator. |

## Return Value

Return value of type **graphStatus**: **GRAPH\_SUCCESS** indicates success; **GRAPH\_FAILED** indicates failure.

## Constraints

None
