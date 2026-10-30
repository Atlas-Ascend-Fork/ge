# ExecuteGraphWithStreamAsync

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-26T13:13:43.263Z pushedAt=2026-10-28T07:42:19.795Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Asynchronously executes the Graph corresponding to the specified ID and outputs the execution result.

This API and [RunGraphWithStreamAsync](RunGraphWithStreamAsync.md) are both used to run the graph corresponding to the specified ID and output the result. The differences from **RunGraphWithStreamAsync** are as follows:

- Before running, this API requires the [CompileGraph](CompileGraph.md) and [LoadGraph](LoadGraph.md) processes (for the asynchronous Graph execution scenario) to be completed.
- The inputs and outputs of this API are of the **gert::Tensor** data type. Both inputs and outputs are memory spaces on the Device, and the memory size must be allocated by the user before running.

    In the following two cases, if the output memory is not allocated, pass null for outputs:

  - If the user has set an external allocator through [RegisterExternalAllocator](RegisterExternalAllocator.md) and the output memory is not allocated, GE calls the external allocator API to allocate the memory. The user must release this memory before the external allocator is destroyed.
  - If the user has not set an external allocator, GE uses the built-in allocator to allocate the memory. The memory lifecycle is consistent with the graph lifecycle. The user must proactively release this memory before the graph is unloaded (before the Session is destroyed or before **GEFinalize** is called), for example, by calling the **outputs.clear\(\)** method. If it is not released in time, undefined behavior will occur.

## Prototype

```c++
Status ExecuteGraphWithStreamAsync(uint32_t graph_id, void *stream,const std::vector<gert::Tensor> &inputs,std::vector<gert::Tensor> &outputs)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to run. |
| stream | Input | Stream on which the graph runs. |
| inputs | Input | Input tensors of the computation graph, which are memory spaces on the device.<br>If the **ge.exec.hostInputIndexes** parameter is specified through **options**, the tensors at the corresponding indexes can be memory spaces on the Host. |
| outputs | Output | Output tensors of the computation graph, which are memory spaces on the device. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: The graph is executed asynchronously.<br>**FAILED**: The graph fails to be executed asynchronously. |

## Constraints

- Before calling this API, allocate the memory required by the tensors.
- Before calling this API, complete the [CompileGraph](CompileGraph.md) and [LoadGraph](LoadGraph.md) processes.
- Before calling this API, create a stream by calling the **aclrtCreateStream** API provided by acl.
- Before obtaining the output execution result, call the **aclrtSynchronizeStream** API provided by acl to ensure that the tasks on the stream have been executed.

    For details about the API, see "Stream Management" in *Runtime API*.

## Examples

See [Asynchronous Graph Execution](../../../../../user_guides/graph_dev/compile_and_run_graph/run_graph_asynchronously.md).
