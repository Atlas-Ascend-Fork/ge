# Proc

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:27:00.097Z pushedAt=2026-11-16T05:03:14.176Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

A user-defined flow func handler function.

## Prototype

```cpp
int32_t Proc(const std::vector<std::shared_ptr<FlowMsg>> &inputMsgs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| inputMsgs | Input | List of input parameters of the operator. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

If an unrecoverable exception occurs, ERROR is returned. In other cases, call SetRetCode to set the error code of the output tensor. If SUCCESS is returned, scheduling is terminated.

## Constraints

None.
