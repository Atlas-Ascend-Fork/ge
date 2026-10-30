# RunGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:21:24.129Z pushedAt=2026-10-28T07:42:19.819Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Synchronously runs the graph with the specified ID and outputs the running result.

This API includes the operations of compiling, loading, and running the graph.

## Prototype

```c++
Status RunGraph(uint32_t graph_id, const std::vector<Tensor>& inputs, std::vector<Tensor>& outputs)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to run. |
| inputs | Input | Input tensors of the computation graph, which are memory spaces allocated on the host.<br>When inputs of the std::vector\<Tensor> type are used as input, each input requires a memory allocation and copy when it enters the data queue. This may cause a performance bottleneck when the inputs are large or numerous. |
| outputs | Output | Output tensors of the computation graph. You do not need to allocate memory space for them. After execution, GE allocates memory and assigns values. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The graph runs successfully.<br>  - **FAILED**: The graph fails to run. |

## Constraints

The inputs correspond to the data nodes in the graph. The index attribute of a data node indicates the position of the corresponding data in the inputs list. That is, you must ensure that the corresponding data can be obtained from inputs according to the index attribute of the data node; otherwise, an error is returned. If the graph has no data nodes, inputs can be empty. The outputs correspond to the user-specified output nodes and match the number and order of the output ports.
