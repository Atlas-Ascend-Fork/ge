# FetchDataFlowGraph (Obtaining All Output FlowMsg)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:22:53.820Z pushedAt=2026-11-16T06:11:08.736Z nl=2 -->

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
| outputs | Output | Pointer to the output FlowMsg of the computation graph. The user does not need to allocate memory space. After execution is complete, GE allocates memory and assigns a value. |
| timeout | Input | Data extraction timeout period, in ms. The value -1 indicates that it never times out. |

## Return Value

The function status results are as follows.

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data obtained successfully.<br>  - FAILED: Failed to obtain data.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
