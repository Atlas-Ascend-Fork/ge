# BalanceConfig Constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:04:48.065Z pushedAt=2026-10-29T02:55:23.189Z nl=2 -->

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
