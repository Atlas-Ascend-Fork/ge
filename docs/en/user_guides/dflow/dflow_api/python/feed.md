# feed

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:25:44.564Z pushedAt=2026-11-16T07:54:08.243Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Feeds data into a Graph, supporting any serializable input.

## Prototype

```python
feed(feed_dict, timeout=-1, partial_inputs = False) -> int
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| feed_dict | Dict[FlowData, Any] | The key is a FlowData node, and the value can be any serializable input.<br>When the input is connected to an NPU model, it must be of the dataflow Tensor type. |
| timeout | int | Data input timeout, in ms. The value range is [0, 2147483647). A value of -1 indicates that it never times out. |
| partial_inputs | bool | Whether feed_dict supports partial inputs of the model for each call to the feed API. The values are as follows.<br><br>  - True: feed_dict can contain only partial inputs of the model.<br>  - False: feed_dict must contain all inputs of the model.<br><br>The default value is False. |

## Return Value

Returns 0 in normal scenarios.

Returns a specific error code and prints an error log in abnormal scenarios.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
graph.feed(...)
```

## Constraints

When a non-dataflow Tensor type is used as the input, the decorators @pyflow and @method must be used together.
