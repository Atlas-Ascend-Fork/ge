# set\_attr

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:11:00.540Z pushedAt=2026-10-29T03:21:31.750Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the attributes of a FlowNode.

## Prototype

```python
set_attr(attr_name, attr_value)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| attr_name | str | Attribute name.<br>The following three attributes are supported:<br><br>  - _flow_attr: a BOOL attribute that can be set to true or false. true indicates that the flow attribute exists, and false indicates that it does not.<br>  -_flow_attr_depth: an INT attribute that specifies the queue depth. The value range is greater than or equal to 2. If it is not configured, the default depth is 128.<br>  -_flow_attr_enqueue_policy: a STRING attribute that specifies the enqueue policy. Only "FIFO" and "OVERWRITE" are supported. "FIFO" indicates that queue data is enqueued in order and waits for dequeue when the queue is full. "OVERWRITE" indicates that enqueue does not wait and data is overwritten in a circular manner. If it is not configured, the default policy is FIFO. |
| attr_value | Union[bool, str, int] | Attribute value. |

## Return Value

Returns None in normal scenarios.

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
pp = df.FuncProcessPoint(...)
flow_node = df.FlowNode(...)
flow_node.set_attr("_flow_attr",True)
flow_node.set_attr("_flow_attr_depth",5)
flow_node.set_attr("_flow_attr_enqueue_policy","FIFO")
```

## Constraints

To set depth and policy through \_flow\_attr\_depth and \_flow\_attr\_enqueue\_policy, you must first enable \_flow\_attr; otherwise, the settings do not take effect.
