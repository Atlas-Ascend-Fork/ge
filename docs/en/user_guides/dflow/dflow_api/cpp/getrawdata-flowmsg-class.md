# GetRawData (FlowMsg Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:40:36.674Z pushedAt=2026-10-29T01:46:01.177Z nl=2 -->

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
