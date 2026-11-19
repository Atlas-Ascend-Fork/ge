# UpdateOutputDesc

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:32:42.639Z pushedAt=2026-10-28T07:42:19.716Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header and Library Files

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Updates the TensorDesc of an Output based on the Output name of the operator.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. Use the API with a non-string data type instead.

```c++
graphStatus UpdateOutputDesc(const std::string &name, const TensorDesc &tensor_desc)
graphStatus UpdateOutputDesc(const char_t *name, const TensorDesc &tensor_desc)
graphStatus UpdateOutputDesc(const uint32_t index, const TensorDesc &tensor_desc)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the operator Output. |
| tensor_desc | Input | **TensorDesc** object. |
| index | Input | Index of the operator Output. |

## Return Value

Return type: graphStatus. Returns GRAPH\_SUCCESS if the TensorDesc is updated successfully; otherwise, returns GRAPH\_FAILED.

## Constraints

None
