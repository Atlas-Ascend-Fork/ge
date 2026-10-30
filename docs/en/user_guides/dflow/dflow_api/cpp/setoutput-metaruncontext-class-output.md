# SetOutput (MetaRunContext Class, Output)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:47:58.500Z pushedAt=2026-10-29T02:31:46.630Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the output for the specified index and options. This function is called by the func function.

## Prototype

```cpp
int32_t SetOutput(uint32_t outIdx, std::shared_ptr<FlowMsg> outMsg, const OutOptions &options) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| outIdx | Input | Output index, starting from 0. |
| outMsg | Input | Output information. |
| options | Input | OutOptions corresponding to the output. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
