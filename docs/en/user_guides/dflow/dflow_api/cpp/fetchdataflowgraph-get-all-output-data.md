# FetchDataFlowGraph (Obtaining All Output Data)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:22:39.603Z pushedAt=2026-11-16T06:11:08.738Z nl=2 -->

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
Status FetchDataFlowGraph(uint32_t graph_id, std::vector<Tensor> &outputs, DataFlowInfo &info, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| outputs | Output | Output tensor of the compute graph. You do not need to allocate memory. After execution, GE allocates memory and assigns values. |
| info | Output | Output data flow flag. For details, see [DataFlowInfo Data Type](dataflowinfo-datatype.md). |
| timeout | Input | Timeout for data acquisition, in ms. The value -1 indicates that the operation never times out. |

## Return Value

Function status result

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data obtained successfully.<br>  - FAILED: Failed to obtain data.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
