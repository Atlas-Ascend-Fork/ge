# SetUserData (DataFlowInfo Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:53:32.714Z pushedAt=2026-10-29T02:37:18.989Z nl=2 -->

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
| data | Input | User Data pointer. |
| size | Input | Length of the User Data. Value range: (0, 64]. |
| offset | Input | Offset of the User Data, subject to the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

- 0: SUCCESS.
- other: FAILED.

## Exception Handling

None.

## Constraints

None.
