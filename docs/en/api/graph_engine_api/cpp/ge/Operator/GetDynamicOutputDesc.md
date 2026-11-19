# GetDynamicOutputDesc

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:17:31.107Z pushedAt=2026-10-28T07:42:19.663Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the TensorDesc of the operator's dynamic output based on the combination of name and index.

## Prototype

> [!NOTE]
> APIs with the string data type will be deprecated in later versions. It is recommended to use APIs with a non-string data type.

```c++
TensorDesc GetDynamicOutputDesc(const std::string &name, uint32_t index) const
TensorDesc GetDynamicOutputDesc(const char_t *name, uint32_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the operator's dynamic output. |
| index | Input | Number of the operator's dynamic output, starting from 1. |

## Return Value

If the operation succeeds, the TensorDesc of the operator's dynamic output is returned. If the operation fails, a TensorDesc object constructed by default is returned, in which DataType is set to DT\_FLOAT (indicating the float type) and Format is set to FORMAT\_NCHW (indicating NCHW).

## Constraints

None
