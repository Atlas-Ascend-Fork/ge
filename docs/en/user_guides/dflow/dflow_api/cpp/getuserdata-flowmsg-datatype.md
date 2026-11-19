# GetUserData (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:08:41.312Z pushedAt=2026-11-16T03:42:53.699Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains user information.

## Prototype

```cpp
Status GetUserData(void *data, size_t size, size_t offset = 0U) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data | Input/Output | User data pointer |
| size | Input | Length of the user data. Value range: (0, 64]. |
| offset | Input | Offset of the user data, which must meet the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

- 0: SUCCESS.
- other: FAILED.

## Exception Handling

None.

## Constraints

None.

The values of the size and offset parameters of this API must be consistent with those of size and offset in [SetUserData](setuserdata-dataflowinfo-datatype.md).
