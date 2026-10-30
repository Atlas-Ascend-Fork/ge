# RegisterFlowFunc

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:07:32.328Z pushedAt=2026-10-29T02:04:22.947Z nl=2 -->

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
| flowFuncName | Input | Name of the flow func. It cannot be set to NULL and must end with “\0”. |
| func | Input | Creation function of the flow func. |

## Return Value

- true
- false

## Exception Handling

None.

## Constraints

None.
