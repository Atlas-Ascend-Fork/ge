# UpdateDynamicInputDesc

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:31:26.384Z pushedAt=2026-10-28T07:42:19.712Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Updates the TensorDesc of the operator's dynamic input based on the combination of name and index.

## Prototype

> [!NOTE]
> The API whose data type is string will be deprecated in later versions. You are advised to use the API whose data type is not string.

```c++
graphStatus UpdateDynamicInputDesc(const std::string &name, uint32_t index, const TensorDesc &tensor_desc)
graphStatus UpdateDynamicInputDesc(const char_t *name, uint32_t index, const TensorDesc &tensor_desc)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the operator's dynamic input. |
| index | Input | Number of the operator's dynamic input, starting from 0. |
| tensor_desc | Input | **TensorDesc** object. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS if the dynamic input is updated successfully; otherwise, returns GRAPH\_FAILED.

## Constraints

None
