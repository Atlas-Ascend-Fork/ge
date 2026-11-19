# set\_alias

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:23:30.730Z pushedAt=2026-11-17T03:39:12.027Z nl=2 -->

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
