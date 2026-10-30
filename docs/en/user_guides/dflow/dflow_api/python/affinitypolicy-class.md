# AffinityPolicy Class

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:02:59.745Z pushedAt=2026-10-29T02:54:53.987Z nl=2 -->

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
ROW_AFFINITY = (fw.AffinityPolicy.ROW_AFFINITY)   # Row affinity: routes data with the same row to the same node.
COL_AFFINITY = (fw.AffinityPolicy.COL_AFFINITY)      # Column affinity: routes data with the same column to the same node.
def __init__(self, inner_type):
    self.inner_type = inner_type
```

> [!NOTE]Note
> The row affinity and column affinity policies are used only to set the balanced distribution of [BalanceConfig](balanceconfig-ctor.md).

## Parameters

None

## Return Value

None

## Exception Handling

None

## Constraints

None
