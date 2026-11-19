# SetBalanceWeight

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:53:25.416Z pushedAt=2026-11-16T06:00:31.089Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets balance distribution weight information.

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
