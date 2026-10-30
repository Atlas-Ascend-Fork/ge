# SetBalanceScatter

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:14:09.130Z pushedAt=2026-10-29T02:09:08.738Z nl=2 -->

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
