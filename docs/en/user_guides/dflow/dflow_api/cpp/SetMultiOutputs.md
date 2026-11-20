# SetMultiOutputs

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:03:00.598Z pushedAt=2026-11-16T06:06:10.364Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the outputs of the specified index and options in batches. This function is called by the func function.

## Prototype

```cpp
int32_t SetMultiOutputs(uint32_t outIdx, const std::vector<std::shared_ptr<FlowMsg>> &outMsgs, const OutOptions &options) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| outIdx | Input | Output index, starting from 0. |
| outMsgs | Input | List of output messages. |
| options | Input | OutOptions corresponding to the output. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
