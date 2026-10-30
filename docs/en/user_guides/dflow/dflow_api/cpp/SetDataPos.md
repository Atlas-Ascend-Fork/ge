# SetDataPos

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:40:39.589Z pushedAt=2026-10-29T02:14:35.230Z nl=2 -->

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
