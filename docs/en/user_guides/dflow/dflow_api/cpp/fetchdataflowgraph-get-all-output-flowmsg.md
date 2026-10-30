# FetchDataFlowGraph (Obtaining All Output FlowMsg)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:53:52.757Z pushedAt=2026-10-29T01:18:34.423Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the graph output data.

## Prototype

```cpp
Status FetchDataFlowGraph(uint32_t graph_id, std::vector<FlowMsgPtr> &outputs, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| outputs | Output | Pointer to the output FlowMsg of the computation graph. The user does not need to allocate memory. After execution, GE allocates memory and assigns the value. |
| timeout | Input | Timeout for data extraction, in ms. The value -1 indicates that the operation never times out. |

## Return Value

The function status results are as follows.

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data obtained successfully.<br>  - FAILED: Failed to obtain data.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
