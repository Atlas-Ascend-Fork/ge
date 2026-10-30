# Reshape

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:09:11.561Z pushedAt=2026-10-29T02:05:33.197Z nl=2 -->

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
| shape | Input | Target shape to be changed to.<br>The number of elements in the shape must be the same as that in the original shape. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see the error codes.

## Exception Handling

None.

## Constraints

If a reshape operation is performed on the input, it may affect the normal execution of other nodes that use this input.
