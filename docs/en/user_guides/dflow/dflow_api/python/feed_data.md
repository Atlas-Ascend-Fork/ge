# feed\_data

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:19:45.460Z pushedAt=2026-10-29T03:06:39.828Z nl=2 -->

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
| timeout | int | Data input timeout, in ms. The value ranges from 0 to 2147483647 (exclusive). The value -1 indicates that the operation never times out. |
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
