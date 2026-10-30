# set\_contains\_n\_mapping\_node

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:12:18.647Z pushedAt=2026-10-29T03:21:47.460Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets whether the FlowGraph contains an n\_mapping node. An n\_mapping node refers to a node that meets any of the following conditions.

- One input produces non-single (multiple, zero, or variable) outputs
- Multiple inputs produce one output
- Multiple inputs produce non-single (multiple, zero, or variable) outputs

## Prototype

```python
set_contains_n_mapping_node(self, contains_n_mapping_node: bool)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| contains_n_mapping_node | bool | Whether the graph contains an n_mapping node.<br>If the graph contains at least one n_mapping processing node, set this parameter to True. The default value is False. |

## Return Value

None.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
graph.set_contains_n_mapping_node(True)
```

## Constraints

None
