# SetOutput (MetaContext Class, Tensor)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:47:56.956Z pushedAt=2026-10-29T02:31:37.625Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the tensor of the output at the specified index. This function is called by [Proc](Proc.md).

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

- 0: Success.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
