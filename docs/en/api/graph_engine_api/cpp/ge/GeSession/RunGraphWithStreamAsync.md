# RunGraphWithStreamAsync

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-16T13:54:13.160Z pushedAt=2026-10-28T07:40:22.665Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Asynchronously runs the graph corresponding to the specified ID and outputs the execution result.

- This API is mutually exclusive with [RunGraph](./RunGraph.md) and [RunGraphAsync](./RunGraphAsync.md). If [LoadGraph](./LoadGraph.md) has not been executed to load the graph before this API is called, this API automatically calls LoadGraph to complete the loading.
- Both this API and RunGraph run the graph corresponding to the specified ID and output the result. Unlike RunGraph, this API:
  - Runs asynchronously.
  - Both inputs and outputs are memory space on the Device, and the user must allocate the memory before running. In the following two cases, if the output memory is not allocated, pass null for outputs:
    - If the user has set an external allocator through [RegisterExternalAllocator](./RegisterExternalAllocator.md) and the output memory is not allocated, GE calls the external allocator's API to allocate memory, and the user must release this memory before the external allocator is destroyed.
    - If the user has not set an external allocator, GE uses the built-in allocator to allocate memory. The memory lifetime is consistent with the graph lifetime, and the user must proactively release this memory before the graph is unloaded (before the Session is destroyed or before GEFinalize is called), for example, by calling the outputs.clear\(\) method. If it is not released in time, undefined behavior will occur.

## Prototype

```c++
Status RunGraphWithStreamAsync(uint32_t graph_id, void *stream,const std::vector<gert::Tensor> &inputs,std::vector<gert::Tensor> &outputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **graph_id** | Input | ID of the graph. |
| **stream** | Input | Stream on which the graph runs. |
| **inputs** | Input | Input tensors of the computation graph, which are memory spaces on the Device. <br>If the **ge.exec.hostInputIndexes** parameter is specified through **options**, the **gert::Tensor** at the corresponding index can be a memory space on the Host. |
| **outputs** | Output | Output tensors of the computation graph, which are memory spaces on the Device. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | **Status** | **SUCCESS**: The graph is executed asynchronously. <br>**FAILED**: The graph fails to be executed asynchronously. |

## Constraints

- Before calling this API, allocate the memory required by **gert::Tensor** first.
- The storage address of the Tensor on the device must be 32-byte aligned; otherwise, undefined errors may occur.
- Before calling this API, create a stream through the **aclrtCreateStream** API. Before obtaining the output execution result, ensure that the tasks on the stream have been executed through the **aclrtSynchronizeStream** API.

    For details about the API, see "Stream Management" in *Runtime API*.

## Examples

See [asynchronous execution of a graph](../../../../../user_guides/graph_dev/compile_and_run_graph/run_graph_asynchronously.md).
