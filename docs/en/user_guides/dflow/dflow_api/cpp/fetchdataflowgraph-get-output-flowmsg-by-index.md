# FetchDataFlowGraph (Obtaining Output FlowMsg by Index)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:54:41.438Z pushedAt=2026-10-29T01:19:10.025Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the graph output data by index.

## Prototype

```cpp
Status FetchDataFlowGraph(uint32_t graph_id, const std::vector<uint32_t> &indexes, std::vector<FlowMsgPtr> &outputs,  int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| indexes | Input | Indexes of the output data. They must not be duplicated. The value range is [0, N-1], where N indicates the number of output data items. One or more indexes are supported.<br>Example: {0,2} indicates obtaining the first and third output data items. |
| outputs | Output | Pointer to the output FlowMsg of the computation graph. The user does not need to allocate memory space. After execution is complete, GE allocates memory and assigns a value. |
| timeout | Input | Data extraction timeout period, in ms. The value -1 indicates that it never times out. |

## Return Value

The function status results are as follows.

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data obtained successfully.<br>  - FAILED: Failed to obtain data.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
