# ResetFlowFuncState (MetaMultiFunc Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:09:13.599Z pushedAt=2026-10-29T02:05:28.329Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

In a fault recovery scenario, quickly reset FlowFunc to the initialized state.

## Prototype

```cpp
int32_t ResetFlowFuncState(const std::shared_ptr<MetaParams> &params)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| params | Input | Parameter information for resetting the multi-func state. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

This API is a virtual function. If the user's FlowFunc does not implement the ResetFlowFuncState function, the framework returns FLOW\_FUNC\_ERR\_NOT\_SUPPORT by default (indicating that the current FlowFunc does not support this operation), deletes the originally created FlowFunc, creates a new FlowFunc, and then calls the [Init](init-metamultifunc-class.md) API for initialization.
