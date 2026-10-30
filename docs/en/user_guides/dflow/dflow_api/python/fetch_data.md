# fetch\_data

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:20:26.633Z pushedAt=2026-10-29T03:06:52.992Z nl=2 -->

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
| indexes | List[int] | Indexes of the output data. They cannot be duplicated. The value range is [0~N-1], where N indicates the number of output data items. One or more indexes are supported. The default value is None, indicating that all outputs are obtained.<br>Example: [0,2] indicates that the first and third output data items are obtained. |
| timeout | int | Data extraction timeout period, in ms. The value range is [0, 2147483647). The value -1 indicates that the operation never times out. |

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
