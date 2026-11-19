# GetVal\(AscendString &value\)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:09:40.537Z pushedAt=2026-11-16T04:46:17.768Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the attribute value of the string type.

## Prototype

```cpp
int32_t GetVal(AscendString &value) const = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Output | Obtained attribute value of the AscendString type. |

## Return Value

- 0: SUCCESS.
- For other return values, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
