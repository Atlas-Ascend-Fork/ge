# fetch\_data

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:26:21.084Z pushedAt=2026-11-16T07:54:14.119Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the Graph output data.

## Prototype

```python
fetch_data(indexes=None, timeout=-1)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| indexes | List[int] | Indexes of the output data. The indexes cannot be duplicated. The value range is [0, N-1], where N indicates the number of output data items. One or more indexes are supported. The default value is None, indicating that all outputs are obtained.<br>Example: [0,2] indicates obtaining the first and third output data items. |
| timeout | int | Timeout for data extraction, in ms. The value range is [0, 2147483647). The value -1 indicates that the operation never times out. |

## Return Value

Returns Tuple\[List\["Tensor"\], FlowInfo, int\].

In normal scenarios, the last return value of the tuple is 0, indicating success.

In abnormal scenarios, the last return value of the tuple indicates the specific error code.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
graph.feed_data(...)
graph.fetch_data(...)
```

## Constraints

None
