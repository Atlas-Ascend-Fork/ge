# SetAffinityPolicy

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:52:28.249Z pushedAt=2026-11-16T06:00:01.708Z nl=2 -->

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
