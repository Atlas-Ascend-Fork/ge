# DynamicInputRegister

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:14:04.849Z pushedAt=2026-10-28T07:42:19.653Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Registers dynamic input for an operator.

## Prototype

```c++
void DynamicInputRegister(const char_t *name, const uint32_t num, bool is_push_back = true)
void DynamicInputRegister(const char_t *name, const uint32_t num, const char_t *datatype_symbol, bool is_push_back = true)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Dynamic input name of the operator. |
| num | Input | Number of dynamic inputs to add. |
| datatype_symbol | Input | Data type of the dynamic input. |
| is_push_back | Input | - **true** indicates appending dynamic input at the end.<br>  - **false** indicates appending dynamic input at the beginning. |

## Return Value

None

## Constraints

None
