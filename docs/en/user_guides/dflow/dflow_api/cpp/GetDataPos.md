# GetDataPos

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T02:48:20.768Z pushedAt=2026-11-16T03:36:21.226Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the position of the output activation in the weight matrix.

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
