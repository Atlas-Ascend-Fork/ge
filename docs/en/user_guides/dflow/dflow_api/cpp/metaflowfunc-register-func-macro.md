# MetaFlowFunc Registration Function Macro

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:21:50.168Z pushedAt=2026-11-16T04:58:29.980Z nl=2 -->

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
