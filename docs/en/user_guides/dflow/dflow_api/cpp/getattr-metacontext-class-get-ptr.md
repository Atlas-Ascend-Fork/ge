# GetAttr (MetaContext Class, Obtaining a Pointer)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:31:56.625Z pushedAt=2026-11-16T06:11:08.778Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains a pointer to the AttrValue type by attribute name. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md).

## Prototype

```cpp
std::shared_ptr<const AttrValue> GetAttr(const char  *attrName) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attrname | Input | Attribute name. |

## Return Value

Pointer to the obtained AttrValue object.

## Exception Handling

None.

## Constraints

None.
