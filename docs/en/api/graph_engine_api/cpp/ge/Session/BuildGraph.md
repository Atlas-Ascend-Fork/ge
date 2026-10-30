# BuildGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:12:32.117Z pushedAt=2026-10-28T07:42:19.790Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Synchronously compiles the graph corresponding to the specified ID to generate an executable model.

## Prototype

```c++
Status BuildGraph(uint32_t graph_id, const std::vector<InputTensorInfo> &inputs)
Status BuildGraph(uint32_t graph_id, const std::vector<ge::Tensor> &inputs)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |
| inputs | Input | Input data of the current graph.<br>For the definition of the **InputTensorInfo** struct, see [InputTensorInfo](../InputTensorInfo.md). |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: The graph is compiled successfully.<br>**FAILED**: The graph fails to be compiled. |

## Constraints

None
