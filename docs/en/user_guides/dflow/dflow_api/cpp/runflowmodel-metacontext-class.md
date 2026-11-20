# RunFlowModel (MetaContext Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:49:44.268Z pushedAt=2026-11-16T05:58:35.030Z nl=2 -->

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
| modelKey | Input | Key of the specified model, which is the same as the name specified in AddInvokedClosure. |
| inputMsgs | Input | Input provided to the model. |
| outputMsgs | Output | Output result of the model execution. |
| timeout | Input | Timeout for waiting for model execution, in ms. -1 indicates no timeout. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
