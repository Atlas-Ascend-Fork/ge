# BalanceConfig Constructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:08:02.626Z pushedAt=2026-11-16T07:44:22.755Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Constructor of BalanceConfig.

## Prototype

```python
__init__(self, row_num: int, col_num: int, affinity_policy: AffinityPolicy = AffinityPolicy.NO_AFFINITY) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| row_num | Input | Number of rows in the weight matrix |
| col_num | Input | Number of columns in the weight matrix |
| affinity_policy | Input | Affinity policy. The default is non-affinity. |

## Return Value

Returns a BalanceConfig object.

## Exception Handling

None

## Constraints

None
