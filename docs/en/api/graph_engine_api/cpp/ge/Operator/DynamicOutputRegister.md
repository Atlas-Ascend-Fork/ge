# DynamicOutputRegister

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:15:13.734Z pushedAt=2026-10-28T07:42:19.657Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Registers dynamic outputs of an operator.

## Prototype

```c++
void DynamicOutputRegister(const char_t *name, const uint32_t num, bool is_push_back = true)
void DynamicOutputRegister(const char_t *name, const uint32_t num, const char_t *datatype_symbol, bool is_push_back = true)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the dynamic output of the operator. |
| num | Input | Number of dynamic outputs to add. |
| datatype_symbol | Input | Data type of the dynamic output. |
| is_push_back | Input | - **true**: appends the dynamic output to the tail.<br>  - **false**: appends the dynamic output to the head. |

## Return Value

None

## Constraints

None
