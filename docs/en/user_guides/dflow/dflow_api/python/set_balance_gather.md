# set\_balance\_gather

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:11:31.854Z pushedAt=2026-10-29T03:21:38.337Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the balance gather property of a node. A UDF with the balance gather property can use balance options to set load-balanced affinity output.

## Prototype

```python
set_balance_gather(self) -> None
```

## Parameters

None

## Return Value

Returns None.

## Examples

```python
import dataflow as df
flow_node = df.FlowNode(...)
flow_node.set_balance_gather()
```

## Constraints

It is mutually exclusive with the [set\_balance\_scatter](set_balance_scatter.md) API.
