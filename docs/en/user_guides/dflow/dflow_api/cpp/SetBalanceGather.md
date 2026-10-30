# SetBalanceGather

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:13:53.145Z pushedAt=2026-10-29T02:08:58.939Z nl=2 -->

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

Returns the FlowNode node with the attribute set.

## Exception Handling

None.

## Constraints

It is mutually exclusive with the [SetBalanceScatter](SetBalanceScatter.md) API.
