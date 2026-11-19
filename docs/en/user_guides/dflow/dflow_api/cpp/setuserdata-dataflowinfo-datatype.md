# SetUserData (DataFlowInfo Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T04:09:16.457Z pushedAt=2026-11-16T06:07:59.986Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets user information.

## Prototype

```cpp
Status SetUserData(const void *data, size_t size, size_t offset = 0U)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data | Input | Pointer to the user data. |
| size | Input | Length of the user data. Value range: (0, 64]. |
| offset | Input | Offset of the user data, which must meet the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

- 0: SUCCESS.
- other: FAILED.

## Exception Handling

None.

## Constraints

None.
