# AffinityPolicy Class

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:05:11.229Z pushedAt=2026-11-16T07:43:47.199Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Defines the affinity policy enumeration.

## Prototype

```python
NO_AFFINITY = (fw.AffinityPolicy.NO_AFFINITY)         # No affinity is required.
ROW_AFFINITY = (fw.AffinityPolicy.ROW_AFFINITY)   # Row affinity, which routes data with the same row to the same node.
COL_AFFINITY = (fw.AffinityPolicy.COL_AFFINITY)      # Column affinity, which routes data with the same column to the same node.
def __init__(self, inner_type):
    self.inner_type = inner_type
```

> [!NOTE]Note
>The row affinity and column affinity policies are used only to set the balanced distribution of [BalanceConfig](balanceconfig-ctor.md).

## Parameters

None

## Return Value

None

## Exception Handling

None

## Constraints

None
