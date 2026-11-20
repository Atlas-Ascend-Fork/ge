# GetRawData (FlowMsg Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:00:12.254Z pushedAt=2026-11-16T03:40:14.331Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Gets the data pointer and data size corresponding to data of the RawData type.

## Prototype

```cpp
virtual Status GetRawData(void *&dataPtr, uint64_t &dataSize) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataPtr | Output | Data pointer of RawData |
| dataSize | Output | Data size of RawData |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see the error codes.

## Exception Handling

Data of a non-RawData type FLOW\_FUNC\_FAILED.

## Constraints

None.
