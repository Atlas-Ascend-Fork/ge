# UpdateDynamicOutputDesc

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:31:39.086Z pushedAt=2026-10-28T07:42:19.713Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Updates the TensorDesc of the operator's dynamic Output based on the combination of name and index.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. You are advised to use the API with a non-string data type.

```c++
graphStatus UpdateDynamicOutputDesc(const std::string &name, uint32_t index, const TensorDesc &tensor_desc)
graphStatus UpdateDynamicOutputDesc(const char_t *name, uint32_t index, const TensorDesc &tensor_desc)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the operator's dynamic Output. |
| index | Input | Number of the operator's dynamic Output. |
| tensor_desc | Input | **TensorDesc** object. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS if the dynamic Output is updated successfully; otherwise, returns GRAPH\_FAILED.

## Constraints

None
