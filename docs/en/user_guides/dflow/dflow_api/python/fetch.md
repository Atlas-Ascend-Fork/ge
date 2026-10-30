# fetch

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:20:56.935Z pushedAt=2026-10-29T03:06:56.839Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the output data of a Graph. Any serializable output is supported.

## Prototype

```python
fetch(indexes=None, timeout=-1)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| indexes | List[int] | Indexes of the output data. The indexes cannot be duplicated. The value range is [0, N-1], where N indicates the number of output data items. One or more indexes are supported. The default value is None, indicating that all outputs are obtained.<br>Example: [0,2] indicates obtaining the first and third output data items. |
| timeout | int | Timeout for data extraction, in ms. The value range is [0, 2147483647). The value -1 indicates that the operation never times out. |

## Return Value

Returns Tuple\[List\["Any"\], int\].

In normal scenarios, the last return value of the tuple is 0, indicating success.

In abnormal scenarios, the last return value of the tuple indicates the specific error code.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
graph.feed(...)
graph.fetch(...)
```

## Constraints

When a non-dataflow Tensor type is used as the input, the decorators @pyflow and @method must be used together.
