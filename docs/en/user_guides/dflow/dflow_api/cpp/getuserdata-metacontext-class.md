# GetUserData (MetaContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:48:56.777Z pushedAt=2026-10-29T01:48:58.056Z nl=2 -->

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
| data | Input/Output | Pointer to the user data. |
| size | Input | Length of the user data. Value range: (0, 64]. |
| offset | Input | Offset of the user data, which must comply with the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
