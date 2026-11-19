# GetVal\(std::vector<TensorDataType\> &value\)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T03:13:47.981Z pushedAt=2026-11-16T04:50:26.656Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the attribute value of the list TensorDataType type.

## Prototype

```cpp
int32_t GetVal(std::vector<TensorDataType> &value) const = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| Value | Output | Attribute value of the list TensorDataType type to be obtained. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
