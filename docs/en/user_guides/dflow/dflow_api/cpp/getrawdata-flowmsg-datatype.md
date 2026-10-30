# GetRawData (FlowMsg Data Type)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:40:55.379Z pushedAt=2026-10-29T01:46:08.094Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the data pointer and data size corresponding to the data of the RawData type.

## Prototype

```cpp
virtual Status GetRawData(void *&data_ptr, uint64_t &data_size) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataPtr | Output | Data pointer of RawData |
| dataSize | Output | Data size of RawData |

## Return Value

- 0: SUCCESS.
- other: FAILED

## Exception Handling

None.

## Constraints

None.
