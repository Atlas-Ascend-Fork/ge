# GetOpType

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:22:16.457Z pushedAt=2026-10-28T07:42:19.679Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the operator type.

## Prototype

> [!NOTE]
> The API whose data type is string will be deprecated in future versions. Use the API whose data type is not string instead.

```c++
std::string GetOpType() const
graphStatus GetOpType(AscendString &type) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| type | Output | Operator type. |

## Return Value

graphStatus type: GRAPH\_SUCCESS, indicating success; GRAPH\_FAILED, indicating failure.

## Constraints

None
