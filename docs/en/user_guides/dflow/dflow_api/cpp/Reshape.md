# Reshape

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:48:04.326Z pushedAt=2026-11-16T05:54:52.847Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Performs a reshape operation on a Tensor without changing its content.

## Prototype

```cpp
int32_t Reshape(const std::vector<int64_t> &shape)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shape | Input | Target shape to be changed to.<br>The number of elements in shape must be the same as that in the original shape. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see the error codes.

## Exception Handling

None.

## Constraints

If a reshape operation is performed on the input, it may affect the normal execution of other nodes that use this input.
