# SetBalanceWeight

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:39:21.990Z pushedAt=2026-10-29T02:10:43.875Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the balanced distribution weight information.

## Prototype

```cpp
void SetBalanceWeight(const BalanceWeight &balanceWeight) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| balanceWeight | Input | Balanced distribution weight information.<br>struct BalanceWeight {<br>int32_t rowNum = 0;  // Number of rows in the weight matrix<br>int32_t colNum = 0;  // Number of columns in the weight matrix<br>const int32_t *matrix = nullptr;  // Weight matrix, an array with rowNum rows and colNum columns. If the value is null, it indicates a matrix whose values are all 1.<br>}; |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
