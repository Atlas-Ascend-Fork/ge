# SetBalanceGather

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:52:37.770Z pushedAt=2026-11-16T06:00:17.821Z nl=2 -->

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

```cpp
FlowNode &SetBalanceGather()
```

## Parameters

None.

## Return Value

Returns the FlowNode node with the properties set.

## Exception Handling

None.

## Constraints

It is mutually exclusive with the [SetBalanceScatter](SetBalanceScatter.md) API.
