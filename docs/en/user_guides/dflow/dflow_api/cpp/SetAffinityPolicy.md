# SetAffinityPolicy

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:13:24.945Z pushedAt=2026-10-29T02:08:55.751Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the balanced distribution affinity.

## Prototype

```cpp
void SetAffinityPolicy(AffinityPolicy affinityPolicy) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| affinityPolicy | Input | Affinity.<br>enum class AffinityPolicy : int32_t {<br>NO_AFFINITY = 0,  // No affinity required<br>ROW_AFFINITY = 1,  // Row affinity<br>COL_AFFINITY = 2,  // Column affinity<br>}; |

## Return Value

None.

## Exception Handling

None.

## Constraints

None.
