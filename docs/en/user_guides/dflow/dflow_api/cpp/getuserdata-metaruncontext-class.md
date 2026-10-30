# GetUserData (MetaRunContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:49:37.618Z pushedAt=2026-10-29T01:49:08.664Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains user data. This function is called by [Proc](Proc.md).

## Prototype

```cpp
int32_t GetUserData(void *data, size_t size, size_t offset = 0U) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data | Input/Output | User data pointer. |
| size | Input | Length of the user data. Value range: (0, 64]. |
| offset | Input | Offset value of the user data, subject to the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
