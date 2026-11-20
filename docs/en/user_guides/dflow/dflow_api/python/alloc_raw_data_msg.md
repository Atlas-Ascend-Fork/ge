# alloc\_raw\_data\_msg

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:06:40.942Z pushedAt=2026-11-16T07:43:54.625Z nl=2 -->

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
