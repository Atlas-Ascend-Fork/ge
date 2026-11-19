# MetaMultiFunc Registration Function Macro

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:23:01.103Z pushedAt=2026-11-16T04:59:37.847Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Registers the implementation class of MetaMultiFunc.

## Prototype

```cpp
FLOW_FUNC_REGISTRAR(clazz)
```

> [!NOTE]
>The following is an example of using this function:
>FLOW\_FUNC\_REGISTRAR\(UserFlowFunc\).RegProcFunc\("xxx\_func", &UserFlowFunc::Proc1\).RegProcFunc\("xxx\_func", &UserFlowFunc::Proc2\);

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| clazz | Input | Class name of the MetaMultiFunc class implementation. |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
