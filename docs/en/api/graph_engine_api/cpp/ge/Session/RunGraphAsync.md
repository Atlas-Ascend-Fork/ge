# RunGraphAsync

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:21:41.211Z pushedAt=2026-10-28T07:42:19.820Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Asynchronously runs the graph corresponding to the specified ID and outputs the running result.

This API and [RunGraph](RunGraph.md) both run the graph corresponding to the specified ID and output the result. Unlike RunGraph, this API:

- Runs asynchronously.
- Uses a different data format to carry data, but the meaning is the same.
- You can obtain the graph computation result through the callback function RunAsyncCallback, that is, you define the function RunAsyncCallback yourself. In this callback function, when status is success, you can process the data.

## Prototype

```c++
Status RunGraphAsync(uint32_t graph_id, const std::vector<ge::Tensor> &inputs, RunAsyncCallback callback)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the graph to run. |
| inputs | Input | Input tensors of the computation graph.<br>When inputs of the std::vector\<Tensor> type are used as input, each input requires a memory allocation and copy when it enters the data queue. A performance bottleneck may occur when the inputs are large or numerous. |
| callback | Input | Callback function corresponding to the current graph. For details, see the RunAsyncCallback content below the table. |

```c++
using Status = uint32_t;
using RunAsyncCallback = std::function<void(Status, std::vector<ge::Tensor> &)>;
```

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **GE_CLI_GE_NOT_INITIALIZED**: GE is not initialized.<br>**SUCCESS**: The graph runs asynchronously successfully.<br>**FAILED**: The graph fails to run asynchronously. |

## Constraints

None

## Examples

1. Call the AddGraph API to load the subgraph.

    ```c++
    std::map <AscendString, AscendString> options;
    ge::GeSession *session = new GeSession(options);

    uint32_t graph_id = 0;
    ge::Graph graph;
    Status ret = session->AddGraph(graph_id, graph);
    ```

2. Define a custom RunAsyncCallback to determine how to process the data, for example:

    ```c++
    void CallBack(Status result, std::vector<ge::Tensor> &out_tensor) {
    if(result == ge::SUCCESS) {
    // Read the out_tensor data and process it as needed.
    for(auto &tensor : out_tensor) {
      auto data = tensor.GetData();
      int64_t length = tensor.GetSize();
      }
    }
    }
    ```

3. Define the input data const std::vector\<ge::Tensor\> &inputs for the specified graph.
4. Call the API RunGraphAsync\(graph\_id, inputs, CallBack\).
