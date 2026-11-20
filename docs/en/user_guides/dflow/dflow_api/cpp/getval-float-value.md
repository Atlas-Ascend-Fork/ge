# GetVal\(float &value\)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:10:50.453Z pushedAt=2026-11-16T04:46:34.536Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the property value of the float type.

## Prototype

```cpp
int32_t GetVal(float &value) const = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| Value | Output | Property value of the float type to obtain. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
