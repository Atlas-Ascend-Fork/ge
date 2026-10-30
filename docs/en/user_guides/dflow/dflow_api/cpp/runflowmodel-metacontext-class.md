# RunFlowModel (MetaContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:11:00.351Z pushedAt=2026-10-29T02:08:17.980Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Synchronously executes the specified model. This function is called by [Proc](Proc.md).

## Prototype

```cpp
int32_t RunFlowModel(const char *modelKey, const std::vector<std::shared_ptr<FlowMsg>> &inputMsgs,
std::vector<std::shared_ptr<FlowMsg>> &outputMsgs, int32_t timeout)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelKey | Input | Specified model key, which must be consistent with the name specified in AddInvokedClosure. |
| inputMsgs | Input | Input provided to the model. |
| outputMsgs | Output | Output result of model execution. |
| timeout | Input | Timeout for waiting for model execution, in ms. -1 indicates no timeout. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
