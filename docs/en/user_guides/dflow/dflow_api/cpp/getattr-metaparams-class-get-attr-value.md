# GetAttr (MetaParams Class, Obtaining Attribute Values)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:32:10.422Z pushedAt=2026-11-16T06:11:08.779Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the corresponding attribute value based on the attribute name. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md).

## Prototype

```cpp
int32_t GetAttr(const char *attrName, T &value) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attrName | Input | Attribute name. |
| value | Output | Attribute value. |

## Return Value

- 0: SUCCESS.
- other: FAILED. For details, see [UDF Error Codes](udf-error-code.md).

## Exception Handling

None.

## Constraints

None.
