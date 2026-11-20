# RunGraphAsync

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:54:19.409Z pushedAt=2026-10-28T07:40:22.666Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Asynchronously runs the graph corresponding to the specified ID and outputs the execution result.

- This API is mutually exclusive with [RunGraph](./RunGraph.md)/[RunGraphWithStreamAsync](./RunGraphWithStreamAsync.md). If [LoadGraph](./LoadGraph.md) is not called to load the graph before this API is invoked, this API automatically calls **LoadGraph** to complete the loading.
- Both this API and **RunGraph** execute the graph corresponding to the specified ID and output the result. Unlike **RunGraph**, this API:
  - Runs asynchronously.
  - The user obtains the graph computation result through the callback function **RunAsyncCallbackV2**, that is, the user defines the function **RunAsyncCallbackV2**. In this callback function, when **Status** is **SUCCESS**, the user can process the data.

## Prototype

```c++
Status RunGraphAsync(uint32_t graph_id, const std::vector<gert::Tensor> &inputs, RunAsyncCallbackV2 callback)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the graph. |
| inputs | Input | Input tensors of the computation graph, which can reside on the Host or on the Device.<br>If the data resides on the Host while execution is on the device, each input requires a memory allocation and copy when it enters the data queue, which may cause a performance bottleneck when the inputs are large or numerous. |
| callback | Input | Callback function corresponding to the current graph. The output is Host memory allocated by GE. For details about the callback function, see **RunAsyncCallbackV2** below the table. |

```c++
using Status = uint32_t;
using RunAsyncCallbackV2 = std::function<void(Status, std::vector<gert::Tensor> &)>;
```

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_GE_NOT_INITIALIZED**: GE is not initialized.<br>**SUCCESS**: The asynchronous execution of the graph succeeds.<br>**FAILED**: The asynchronous execution of the graph fails. |

## Constraints

If the input Tensor data resides on the device, the storage address of the Tensor on the device must be 32-byte aligned; otherwise, undefined errors may occur.

## Examples

1. Call the **AddGraph** API to load the subgraph.

    ```c++
    std::map <AscendString, AscendString> options;
    ge::GeSession *session = new GeSession(options);

    uint32_t graph_id = 0;
    ge::Graph graph;
    Status ret = session->AddGraph(graph_id, graph);
    ```

2. Define a custom **RunAsyncCallbackV2** to determine how to process the data, for example:

    ```c++
    void CallBack(Status result, std::vector<gert::Tensor> &out_tensor) {
    if(result == ge::SUCCESS) {
    // Read the out_tensor data and process it as required.
    for(auto &tensor : out_tensor) {
      auto data = tensor.GetAddr();
      int64_t length = tensor.GetSize();
      }
    }
    }
    ```

3. Define the input data const std::vector\<gert::Tensor\> &inputs of the specified graph.
4. Call the API RunGraphAsync\(graph\_id, inputs, CallBack\).
