# SetMarks

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:32:50.403Z pushedAt=2026-10-28T07:40:22.802Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Sets the marks of paired resource operators in the inference context of resource operators.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. It is recommended to use the API with a non-string data type.

```c++
void SetMarks(const std::vector<std::string> &marks)
void SetMarks(const std::vector<AscendString> &marks)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| marks | Input | Marks of resource operators. |

## Return Value

None

## Constraints

None
