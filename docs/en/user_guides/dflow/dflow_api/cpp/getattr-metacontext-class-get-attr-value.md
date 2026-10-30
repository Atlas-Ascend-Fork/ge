# GetAttr (MetaContext Class, Obtaining Attribute Values)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:02:22.919Z pushedAt=2026-10-29T01:32:44.881Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the attribute value based on the attribute name. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md).

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
