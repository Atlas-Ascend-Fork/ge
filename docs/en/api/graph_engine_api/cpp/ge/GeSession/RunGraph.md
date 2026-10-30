# RunGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:53:23.377Z pushedAt=2026-10-28T07:40:22.663Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Synchronously runs the graph corresponding to the specified ID and outputs the running result.

This API is mutually exclusive with [RunGraphAsync](./RunGraph.md)/[RunGraphWithStreamAsync](./RunGraphWithStreamAsync.md). If [LoadGraph](./LoadGraph.md) is not called to load the graph before this API is called, this API automatically calls LoadGraph to complete the loading.

## Prototype

```c++
Status RunGraph(uint32_t graph_id, const std::vector<gert::Tensor>& inputs, std::vector<gert::Tensor>& outputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the graph to be run. |
| inputs | Input | Input tensors of the computation graph, which can reside on the Host or on the Device.<br>If the data resides on the Host while execution is on the Device, memory allocation and copy must be performed once for each input when user input enters the data queue. This may cause a performance bottleneck when the inputs are large or numerous. |
| outputs | Output | Output tensors of the computation graph. The user does not need to allocate memory space. After execution completes, GE allocates Host memory and assigns values. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The graph runs successfully.<br>  - **FAILED**: The graph fails to run. |

## Constraints

- **inputs** corresponds to the data nodes in the graph. The **index** attribute of a data node indicates the position of the corresponding data in the **inputs** list. That is, the user must ensure that the corresponding data can be obtained from **inputs** according to the **index** attribute of the data node; otherwise, an error is returned. If the graph has no data nodes, **inputs** can be empty. The output **outputs** is consistent with the number and order of the output nodes and output ports specified by the user.
- If the passed tensor data resides on the device, the storage address of the tensor on the device must be 32-byte aligned; otherwise, undefined errors may occur.
