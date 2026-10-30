# MetaFlowFunc Registration Function Macro

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:00:42.639Z pushedAt=2026-10-29T01:57:58.910Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Registers the implementation class of MetaFlowFunc.

## Prototype

```cpp
REGISTER_FLOW_FUNC(name, clazz)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| Name | Input | User-defined function name. |
| clazz | Input | Class name of the MetaFlowFunc class implementation. |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
