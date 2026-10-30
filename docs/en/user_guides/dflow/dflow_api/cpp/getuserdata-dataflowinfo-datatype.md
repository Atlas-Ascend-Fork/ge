# GetUserData (DataFlowInfo Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:48:14.894Z pushedAt=2026-10-29T01:48:13.424Z nl=2 -->

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
| data | Input/Output | User Data pointer |
| size | Input | Length of the User Data. Value range: (0, 64]. |
| offset | Input | Offset of the User Data, which must meet the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

- 0: SUCCESS.
- other: FAILED.

## Exception Handling

None.

## Constraints

None.

The values of the size and offset parameters of this API must be consistent with those of size and offset in [SetUserData](setuserdata-dataflowinfo-datatype.md).
