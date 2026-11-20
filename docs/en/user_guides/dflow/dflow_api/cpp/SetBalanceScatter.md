# SetBalanceScatter

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:53:15.583Z pushedAt=2026-11-16T06:00:25.731Z nl=2 -->

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

```cpp
FlowNode &SetBalanceScatter()
```

## Parameters

None.

## Return Value

Returns the FlowNode node with the properties set.

## Exception Handling

None.

## Constraints

Mutually exclusive with the [SetBalanceGather](SetBalanceGather.md) API.
