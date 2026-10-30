# set\_balance\_scatter

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:11:45.527Z pushedAt=2026-10-29T03:21:43.632Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the balance scatter property of a node. A UDF with the balance scatter property can use balance options to set load-balanced output.

## Prototype

```python
set_balance_scatter(self) -> None
```

## Parameters

None

## Return Value

Returns None.

## Examples

```python
import dataflow as df
flow_node = df.FlowNode(...)
flow_node.set_balance_scatter()
```

## Constraints

Mutually exclusive with the [set\_balance\_gather](set_balance_gather.md) API.
