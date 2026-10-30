# alloc\_raw\_data\_msg

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:03:49.575Z pushedAt=2026-10-29T02:55:02.478Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Allocates a contiguous memory block based on the input size to carry FlowMsg of the raw data type.

## Prototype

```python
alloc_raw_data_msg(self, size, align:Optional[int] = 64) -> FlowMsg
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Input | Size of the memory to be allocated, in bytes. |
| align | Input | Alignment size of the applied memory address. Value range: [32, 64, 128, 256, 512, 1024]. The default value is 64. |

## Return Value

Returns a FlowMsg instance on success. Returns None if the allocation fails.

## Exception Handling

None

## Constraints

None
