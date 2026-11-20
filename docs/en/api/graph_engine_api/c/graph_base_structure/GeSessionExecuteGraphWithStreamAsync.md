# GeSessionExecuteGraphWithStreamAsync

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-11-17T15:46:56.646Z pushedAt=2026-11-18T06:43:08.583Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Function Description

Uses the specified Session instance to asynchronously execute the Graph corresponding to the specified ID and output the execution result.

Both this API and [ExecuteGraphWithStreamAsync](../../cpp/ge/Session/ExecuteGraphWithStreamAsync.md) execute the graph corresponding to the specified ID and output the result. This API is primarily intended for forward compatibility with the graph mode scenarios of upper-layer open-source frameworks. If you are using CANN 8.0.RC3 or later, ExecuteGraphWithStreamAsync is recommended.

- Before calling this API, complete the [CompileGraph](../../cpp/ge/Session/CompileGraph.md) and [GeSessionLoadGraph](GeSessionLoadGraph.md) (asynchronous Graph execution scenario) processes.
- The data type of inputs and outputs is gert::Tensor.

## Prototype

```c
ge::Status GeSessionExecuteGraphWithStreamAsync(ge::Session &session, uint32_t graph_id, void *stream, const std::vector<gert::Tensor> &inputs, std::vector<gert::Tensor> &outputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| session | Input | Session instance for loading the graph. |
| graph_id | Input | ID of the subgraph. |
| stream | Input | Stream on which the graph runs. |
| inputs | Input | Input data of the current subgraph, which is the memory space on the Device. |
| outputs | Output | Output data of the current subgraph, which is the memory space on the Device. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: The asynchronous execution of the graph succeeds.<br>FAILED: The asynchronous execution of the graph fails. |

## Constraints

- Before calling this API, determine the memory size allocated on the Device.
- Before calling this API, complete the [CompileGraph](../../cpp/ge/Session/CompileGraph.md) and [GeSessionLoadGraph](GeSessionLoadGraph.md) process.
- Before calling this API, create a Stream by calling the **aclrtCreateStream** API provided by acl.
- Before obtaining the output execution result, call the **aclrtSynchronizeStream** API to ensure that the tasks on the Stream have been executed.

For details about the API, see "Stream Management" in *[Runtime API](https://gitcode.com/cann/runtime/blob/master/docs/zh/api_ref/README.md)*.
