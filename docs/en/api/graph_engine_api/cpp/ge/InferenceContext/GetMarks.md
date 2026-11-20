# GetMarks

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:29:44.196Z pushedAt=2026-10-28T07:40:22.793Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Obtains the marks of paired resource operators in the inference context of resource operator inference.

## Prototype

> [!NOTE]
> The APIs whose data type is string will be deprecated in later versions. It is recommended to use the APIs whose data type is not string.

```c++
const std::vector<std::string> &GetMarks() const
void GetMarks(std::vector<AscendString> &marks) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| marks | Output | Marks of resource operators. |

## Return Value

Marks of resource operators.

## Constraints

None
