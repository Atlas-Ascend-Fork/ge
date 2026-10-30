# GetDataPos

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:05:43.926Z pushedAt=2026-10-28T12:30:42.563Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the position of the output data in the weight matrix.

## Prototype

```cpp
const std::vector<std::pair<int32_t, int32_t>> &GetDataPos() const = 0
```

## Parameters

None.

## Return Value

Position of the output data in the weight matrix. The first value in the pair indicates the corresponding row number, and the second value indicates the corresponding column number.

## Exception Handling

None.

## Constraints

None.
