# set\_alias

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:10:46.748Z pushedAt=2026-10-29T03:21:25.021Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the node alias. When option:ge.experiment.data\_flow\_deploy\_info\_path is used to specify the node deployment location, the flow\_node\_list field can be specified using the alias.

## Prototype

```python
def set_alias(self, name) -> FlowNode
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| name | str | Alias of the FlowNode. |

## Return Value

Returns the current FlowNode.

## Examples

```python
import dataflow as df
flow_node = df.FlowNode(...)
flow_node.set_alias(name="hello")
```

## Constraints

None
