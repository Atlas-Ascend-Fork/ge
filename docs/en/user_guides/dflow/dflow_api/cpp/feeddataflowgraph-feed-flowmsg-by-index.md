# FeedDataFlowGraph (Feeding FlowMsg by Index)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:52:15.192Z pushedAt=2026-10-29T01:16:13.446Z nl=2 -->

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
| indexes | Input | Indexes of the input data. The value range is [0, N-1], where N indicates the number of input data items. The number must be consistent with the number of inputs parameters.<br>Example: {0,2} indicates feeding data to the first and third inputs. |
| inputs | Input | Pointer to the FlowMsg input to the computation graph, which is shared memory allocated on the Host. |
| timeout | Input | Data input timeout, in ms. A value of -1 indicates that it never times out. |

## Return Value

The function status results are as follows.

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data input succeeded.<br>  - FAILED: Data input failed.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
