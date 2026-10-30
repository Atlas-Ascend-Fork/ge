# FeedDataFlowGraph (Feeding Input by Index)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:52:47.443Z pushedAt=2026-10-29T01:17:03.311Z nl=2 -->

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
Status FeedDataFlowGraph(uint32_t graph_id, const std::vector<uint32_t> &indexes, const std::vector<Tensor> &inputs, const DataFlowInfo &info, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| indexes | Input | Indexes of the input data. The value range is [0, N-1], where N indicates the number of input data. The number of indexes must be the same as the number of inputs.<br>Example: {0,2} indicates feeding data to the first and third inputs. |
| inputs | Input | Input tensors of the compute graph, which are memory space allocated on the host. |
| info | Input | Flow flag of the input data. For details, see [DataFlowInfo Data Type](dataflowinfo-datatype.md). |
| timeout | Input | Timeout for data input, in ms. The value -1 indicates that it never times out. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data input succeeded.<br>  - FAILED: Data input failed.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
