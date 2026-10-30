# GetName

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:21:40.103Z pushedAt=2026-10-28T07:42:19.678Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the operator name.

## Prototype

> [!NOTE]
>APIs whose data type is string will be deprecated in later versions. Use APIs whose data type is not string instead.

```c++
std::string GetName() const
graphStatus GetName(AscendString &name) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Output | Operator name. |

## Return Value

**graphStatus** type: **GRAPH\_SUCCESS**, indicating success; **GRAPH\_FAILED**, indicating failure.

## Constraints

None
