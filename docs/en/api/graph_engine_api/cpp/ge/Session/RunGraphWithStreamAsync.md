# RunGraphWithStreamAsync

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-26T13:22:24.406Z pushedAt=2026-10-28T07:42:19.822Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Asynchronously runs the Graph with the specified ID and outputs the running result. **This API includes the operations of compiling, loading, and running the Graph.**

This function and [RunGraph](RunGraph.md) both run the graph with the specified ID and output the result. Unlike RunGraph, this API:

- Runs asynchronously.
- Both inputs and outputs must be memory space on the Device, and you must allocate the memory size before running. In the following two cases, if the output memory is not allocated, pass null for outputs:
  - If you have set an external allocator through [RegisterExternalAllocator](RegisterExternalAllocator.md) and the output memory is not allocated, GE calls the external allocator API to allocate the memory. You must release this memory before the external allocator is destroyed.
  - If you have not set an external allocator, GE uses the built-in allocator to allocate the memory. The memory lifecycle is consistent with the graph lifecycle. You must proactively release this memory before the graph is unloaded (before the Session is destroyed or **GEFinalize** is called), for example, by calling the **outputs.clear\(\)** method. If the memory is not released in time, undefined behavior occurs.

## Prototype

```c++
Status RunGraphWithStreamAsync(uint32_t graph_id, void *stream, const std::vector<Tensor> &inputs,std::vector<Tensor> &outputs)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to run. |
| stream | Input | Stream on which the graph runs. |
| inputs | Input | Input tensors of the computation graph, which are memory space on the device.<br>If the ge.exec.hostInputIndexes parameter is specified through options, the tensors at the corresponding indexes can be memory space on the Host. |
| outputs | Output | Output tensors of the computation graph, which are memory space on the device. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: The graph is run asynchronously.<br>FAILED: The graph fails to run asynchronously. |

## Constraints

- Before calling this API, allocate the memory required by the tensors.
- The storage address of a tensor on the Device must be 32-byte aligned. Otherwise, undefined errors may occur.
- Before calling this API, create a stream by calling **aclrtCreateStream** provided by ACL. Only the scenario where the stream belongs to the default context is supported.
- Before obtaining the output result, call **aclrtSynchronizeStream** to ensure that the tasks on the stream have been executed.

    For details about the API, see "Stream Management" in *Runtime API*.

## Examples

See [Running a Graph Asynchronously](../../../../../user_guides/graph_dev/compile_and_run_graph/run_graph_asynchronously.md).
