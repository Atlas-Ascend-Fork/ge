# feed\_data

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:24:39.517Z pushedAt=2026-11-16T07:54:01.897Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Feeds data into a Graph.

## Prototype

```python
feed_data(feed_dict, flow_info=None, timeout=-1, partial_inputs = False) -> int
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| feed_dict | Dict[FlowData, Union["numpy.ndarray", Tensor, List]] | The key is a FlowData node, and the value is any input that can be converted to numpy.ndarray, or dataflow.tensor.<br>When feed_dict is empty, the flow_flags of flow_info must contain DATA_FLOW_FLAG_EOS, in which case partial_inputs does not take effect. |
| flow_info | FlowInfo | Set FlowInfo as required. For details, see [dataflow.FlowInfo](dataflow-FlowInfo.md). |
| timeout | int | Data input timeout, in ms. The value range is [0, 2147483647). A value of -1 indicates that it never times out. |
| partial_inputs | bool | Whether feed_dict supports partial inputs of the model in each call to the feed_data API. The values are as follows.<br><br>  - True: feed_dict can contain only partial inputs of the model.<br>  - False: feed_dict must contain all inputs of the model.<br><br>The default value is False. |

## Return Value

Returns 0 in normal scenarios.

Returns a specific error code and prints an error log in abnormal scenarios.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
graph.feed_data(...)
```

## Constraints

None
