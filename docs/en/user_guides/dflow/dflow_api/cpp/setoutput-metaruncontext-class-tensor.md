# SetOutput (MetaRunContext Class, Tensor)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:48:38.346Z pushedAt=2026-10-29T02:32:56.566Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the output tensor at the specified index. This function is called by [Proc](Proc.md).

## Prototype

```cpp
int32_t SetOutput(uint32_t outIdx, std::shared_ptr<FlowMsg> outMsg)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| outIdx | Input | Specifies the index of the output. |
| outMsg | Input | Specifies the Msg of the output. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
