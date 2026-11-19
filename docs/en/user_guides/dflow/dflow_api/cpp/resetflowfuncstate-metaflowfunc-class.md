# ResetFlowFuncState (MetaFlowFunc Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:47:23.412Z pushedAt=2026-11-16T05:54:31.205Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

In fault recovery scenarios, quickly reset FlowFunc to the initialized state.

## Prototype

```cpp
int32_t ResetFlowFuncState()
```

## Parameters

None

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

This API is a virtual function. If the user's FlowFunc does not implement the ResetFlowFuncState function, the framework returns FLOW\_FUNC\_ERR\_NOT\_SUPPORT by default (considering that the current FlowFunc does not support this operation), deletes the originally created FlowFunc, creates a new FlowFunc, and then calls the [Init](init-metaflowfunc-class.md) API for initialization.
