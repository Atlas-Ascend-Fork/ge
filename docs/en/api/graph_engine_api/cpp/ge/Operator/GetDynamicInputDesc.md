# GetDynamicInputDesc

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:16:45.147Z pushedAt=2026-10-28T07:42:19.662Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the TensorDesc of the operator's dynamic input based on the combination of name and index.

## Prototype

> [!NOTE]
> APIs whose data type is string will be deprecated in later versions. Use APIs whose data type is not string instead.

```c++
TensorDesc GetDynamicInputDesc(const std::string &name, uint32_t index) const
TensorDesc GetDynamicInputDesc(const char_t *name, uint32_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the operator's dynamic input. |
| index | Input | Index of the operator's dynamic input, starting from 0. |

## Return Value

If obtained successfully, returns the TensorDesc of the operator's dynamic input; otherwise, returns a TensorDesc object constructed by default, where DataType is set to DT\_FLOAT (float type) and Format is set to FORMAT\_NCHW (NCHW).

## Constraints

None
