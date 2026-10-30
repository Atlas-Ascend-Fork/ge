# UpdateInputDesc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:32:05.123Z pushedAt=2026-10-28T07:42:19.714Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Updates the TensorDesc of the input based on the operator input name.

## Prototype

> [!NOTE]
> The API whose data type is string will be deprecated in later versions. You are advised to use the API whose data type is not string.

```c++
graphStatus UpdateInputDesc(const std::string &name, const TensorDesc &tensor_desc)
graphStatus UpdateInputDesc(const char_t *name, const TensorDesc &tensor_desc)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Operator input name. |
| tensor_desc | Input | **TensorDesc** object. |

## Return Value

graphStatus type: Returns GRAPH\_SUCCESS if the TensorDesc is updated successfully; otherwise, returns GRAPH\_FAILED.

## Exception Handling

| Exception Scenario | Description |
| --- | --- |
| No matching name input | The function exits early and returns **GRAPH_FAILED**. |

## Constraints

None
