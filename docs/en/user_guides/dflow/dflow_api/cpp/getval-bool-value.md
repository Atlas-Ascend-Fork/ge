# GetVal\(bool &value\)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:10:16.009Z pushedAt=2026-11-16T04:46:24.426Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtain the bool type attribute value.

## Prototype

```cpp
int32_t GetVal(bool &value) const = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| Value | Output | Obtained bool type attribute value. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
