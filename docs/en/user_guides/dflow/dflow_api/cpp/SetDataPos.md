# SetDataPos

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:56:10.665Z pushedAt=2026-11-16T06:03:28.635Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the position of the output data in the weight matrix.

## Prototype

```cpp
void SetDataPos(const std::vector<std::pair<int32_t, int32_t>> &dataPos) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataPos | Input | Position of the output data in the weight matrix. |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
