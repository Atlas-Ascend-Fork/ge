# GetUserData (MetaRunContext Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:09:25.087Z pushedAt=2026-11-16T04:46:02.216Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the user activation. This function is called by [Proc](Proc.md).

## Prototype

```cpp
int32_t GetUserData(void *data, size_t size, size_t offset = 0U) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data | Input/Output | Pointer to the user data. |
| size | Input | Length of the user data. Value range: (0, 64]. |
| offset | Input | Offset of the user data, which must meet the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
