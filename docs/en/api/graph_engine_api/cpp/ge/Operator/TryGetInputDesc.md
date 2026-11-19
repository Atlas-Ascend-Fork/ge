# TryGetInputDesc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:31:00.220Z pushedAt=2026-10-28T07:42:19.710Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

Obtains the TensorDesc of the operator input based on the input name.

## Prototype

> [!NOTE]
> APIs whose data type is string will be deprecated in later versions. Use APIs whose data type is not string instead.

```c++
graphStatus TryGetInputDesc(const std::string &name, TensorDesc &tensor_desc) const
graphStatus TryGetInputDesc(const char_t *name, TensorDesc &tensor_desc) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| name | Input | Input name of the operator. |
| tensor_desc | Output | Returns the currently configured format of the operator port, which is a TensorDesc object. |

## Return Value

graphStatus type: True, the port exists and the TensorDesc is obtained successfully; False, the port does not exist, the output parameter is empty, and the TensorDesc fails to be obtained.

## Exception Handling

| Exception Scenarios | **Description** |
| --- | --- |
| No matching name input | Returns False. |

## Constraints

None
