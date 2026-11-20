# SetOutput (MetaRunContext Class, Tensor)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:04:34.444Z pushedAt=2026-11-16T06:06:49.286Z nl=2 -->

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

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
