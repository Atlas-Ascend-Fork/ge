# Proc

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:05:32.476Z pushedAt=2026-10-29T02:02:52.749Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Processing function of a user-defined flow func.

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
