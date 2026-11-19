# FeedDataFlowGraph (Feeding All Inputs)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:20:58.037Z pushedAt=2026-11-16T06:11:08.730Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Feeds data into a Graph.

## Prototype

```cpp
Status FeedDataFlowGraph(uint32_t graph_id, const std::vector<Tensor> &inputs, const DataFlowInfo &info, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| inputs | Input | Input tensors of the compute graph, which are memory space allocated on the host. |
| info | Input | Input data flow flag. For details, see [DataFlowInfo Data Type](dataflowinfo-datatype.md). |
| timeout | Input | Timeout period for data input, in ms. The value -1 indicates that it never times out. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data input succeeded.<br>  - FAILED: Data input failed.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
