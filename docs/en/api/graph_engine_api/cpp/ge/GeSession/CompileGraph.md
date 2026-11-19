# CompileGraph

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:46:33.293Z pushedAt=2026-10-28T07:40:22.641Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Synchronously compiles the graph corresponding to the specified ID.

This API does not include the memory resource management required by the model. Instead, it leaves this part of memory management to users. You can use it together with the API for querying the resource usage of the compiled graph and the API for refreshing the memory base address, so as to manage model memory by yourself and gain more flexibility.

After calling this API, you can call [GetCompiledGraphSummary](./GetCompiledGraphSummary.md) to obtain the summary information of the graph compilation result (for example, the memory resource size required for model execution and whether the memory can be refreshed or reused). Based on the queried memory size, you can apply for and manage the memory by yourself. Then use [SetGraphConstMemoryBase](./SetGraphConstMemoryBase.md) and [UpdateGraphFeatureMemoryBase](./UpdateGraphFeatureMemoryBase.md) to set and refresh the memory base address.

## Prototype

```c++
Status CompileGraph(uint32_t graph_id)
Status CompileGraph(uint32_t graph_id, const std::vector<ge::Tensor> &inputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID corresponding to the graph. |
| inputs | Input | Used to update the metadata (**Shape**, **Format**, **DataType**) of the input nodes of the computation graph. It can be empty. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: The graph is compiled successfully.<br>**FAILED**: The graph fails to be compiled.<br>**PARAM_INVALID**: The input graph information is abnormal during graph compilation. |
