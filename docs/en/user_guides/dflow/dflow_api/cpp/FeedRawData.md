# FeedRawData

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:53:03.894Z pushedAt=2026-10-29T01:17:39.025Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Inputs raw data into the Graph.

## Prototype

```cpp
struct RawData {
  const void *addr;
  size_t len;
};
```

```cpp
Status FeedRawData(uint32_t graph_id, const std::vector<RawData> &raw_data_list, const uint32_t index,  const DataFlowInfo &info, int32_t timeout);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph to be executed. |
| raw_data_list | Input | Array consisting of input data pointers and lengths. It can contain one or multiple entries. If multiple entries are provided, the framework automatically merges them into a single piece of data and passes it to the DataFlow graph. |
| index | Input | A specific input of the corresponding DataFlow graph. |
| info | Input | Input data flow flag. For details, see [DataFlowInfo Data Type](dataflowinfo-datatype.md). |
| timeout | Input | Timeout period for data input, in ms. The value -1 indicates that it never times out. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - SUCCESS: Data input succeeded.<br>  - FAILED: Data input failed.<br>  - For other error codes, see [UDF Error Codes](udf-error-code.md). |

## Constraints

None
