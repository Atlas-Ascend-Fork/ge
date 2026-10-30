# FetchDataFlowGraph (Obtaining All Output Data)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:53:36.469Z pushedAt=2026-10-29T01:18:14.378Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: supported
<!-- end id2 -->

## Function

Obtains the output data of a graph.

## Prototype

```cpp
Status FetchDataFlowGraph(uint32_t graph_id, std::vector<Tensor> &outputs, DataFlowInfo &info, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| outputs | Output | Output tensors of the computation graph. The user does not need to allocate memory space. After execution is complete, GE allocates memory and assigns values. |
| info | Output | Output data flow flag. For details, see [DataFlowInfo Data Type](dataflowinfo-datatype.md). |
| timeout | Input | Timeout for data acquisition, in ms. The value -1 indicates that the operation never times out. |

## Return Value

Function status result

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data acquisition succeeded.<br>  - FAILED: Data acquisition failed.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
