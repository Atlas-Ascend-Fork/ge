# set\_balance\_gather

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:24:29.815Z pushedAt=2026-11-17T03:39:12.029Z nl=2 -->

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
