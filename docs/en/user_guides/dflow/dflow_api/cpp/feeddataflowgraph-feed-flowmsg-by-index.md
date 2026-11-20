# FeedDataFlowGraph (Feeding FlowMsg by Index)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:21:03.690Z pushedAt=2026-11-16T06:11:08.731Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Feeds data into the Graph by index.

## Prototype

```cpp
Status FeedDataFlowGraph(uint32_t graph_id, const std::vector<uint32_t> &indexes, const std::vector<FlowMsgPtr> &inputs, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| indexes | Input | Indexes of the input data. The value range is [0, N-1], where N indicates the number of input data. The number of indexes must be the same as the number of inputs.<br>Example: {0,2} indicates feeding data to the first and third inputs. |
| inputs | Input | Pointer to the FlowMsg input to the computation graph, which is shared memory allocated on the Host. |
| timeout | Input | Timeout period for data input, in ms. The value -1 indicates that it never times out. |

## Return Value

The function status results are as follows.

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data input succeeded.<br>  - FAILED: Data input failed.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
