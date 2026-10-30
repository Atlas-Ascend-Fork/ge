# FetchDataFlowGraph (Obtaining Output Data by Index)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:54:22.525Z pushedAt=2026-10-29T01:18:51.294Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: supported
<!-- end id2 -->

## Function

Obtains the graph output data by index.

## Prototype

```cpp
Status FetchDataFlowGraph(uint32_t graph_id, const std::vector<uint32_t> &indexes, std::vector<Tensor> &outputs, DataFlowInfo &info, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| indexes | Input | Indexes of the output data. They cannot be duplicated. The value range is [0, N-1], where N indicates the number of output data items. One or more indexes are supported.<br>Example: {0,2} indicates obtaining the first and third output data items. |
| outputs | Output | Output tensor of the compute graph. You do not need to allocate memory. After execution, GE allocates memory and assigns values. |
| info | Output | Output data flow flag. For details, see [DataFlowInfo Data Type](dataflowinfo-datatype.md). |
| timeout | Input | Timeout for data extraction, in ms. The value -1 indicates that it never times out. |

## Return Value

The function status results are as follows.

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data obtained successfully.<br>  - FAILED: Failed to obtain data.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
