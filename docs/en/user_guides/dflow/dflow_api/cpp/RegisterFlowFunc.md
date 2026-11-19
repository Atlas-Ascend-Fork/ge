# RegisterFlowFunc

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:46:10.227Z pushedAt=2026-11-16T05:53:15.092Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Registers a flow func.

It is not recommended to use this function directly. Instead, use the [MetaFlowFunc Registration Function Macro](metaflowfunc-register-func-macro.md) to register a flow func.

## Prototype

```cpp
FLOW_FUNC_VISIBILITY bool RegisterFlowFunc(const char *flowFuncName, const FLOW_FUNC_CREATOR_FUNC &func) noexcept
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| flowFuncName | Input | Name of the flow func. It cannot be set to NULL and must end with **\0**. |
| func | Input | Creation function of the flow func. |

## Return Value

- true
- false

## Exception Handling

None.

## Constraints

None.
