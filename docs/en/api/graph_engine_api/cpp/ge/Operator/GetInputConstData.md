# GetInputConstData

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-26T12:20:49.441Z pushedAt=2026-10-28T07:42:19.674Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File and Library File

- Header file: \#include <graph/operator.h\>
- Library file: libgraph.so

## Description

If the node corresponding to the Input of the specified operator is a Const node, you can call this API to obtain the data of the Const node.

## Prototype

> [!NOTE]
> The API with the string data type will be deprecated in later versions. Use the API with a non-string data type instead.

```c++
graphStatus GetInputConstData(const std::string &dst_name, Tensor &data) const
graphStatus GetInputConstData(const char_t *dst_name, Tensor &data) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| dst_name | Input | Input name. |
| data | Output | Returns the data tensor of the Const node. |

## Return Value

graphStatus type: If the node corresponding to the Input of the specified operator is a Const node and the data is obtained successfully, GRAPH\_SUCCESS is returned; otherwise, GRAPH\_FAILED is returned.

## Constraints

None
