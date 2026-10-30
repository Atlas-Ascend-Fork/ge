# MetaMultiFunc Registration Function Macro

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:01:44.080Z pushedAt=2026-10-29T01:59:55.622Z nl=2 -->

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
> The following is an example of using this function:
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
