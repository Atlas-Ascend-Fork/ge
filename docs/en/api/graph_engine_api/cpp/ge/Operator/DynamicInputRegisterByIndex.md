# DynamicInputRegisterByIndex

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:15:10.021Z pushedAt=2026-10-28T07:42:19.658Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Registers dynamic inputs for an operator at a specified position.

## Prototype

```c++
void DynamicInputRegisterByIndex(const char_t *name, const uint32_t num, size_t index)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Dynamic Input name of the operator. |
| num | Input | Number of dynamic Inputs to add. |
| index | Input | Index position in which dynamic inputs are added. |

## Return Value

None

## Constraints

None
