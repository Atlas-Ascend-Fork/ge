# GetAttr (MetaContext Class, Obtaining a Pointer)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T12:02:36.222Z pushedAt=2026-10-29T01:37:35.064Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains a pointer of the AttrValue type based on the attribute name. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md).

## Prototype

```cpp
std::shared_ptr<const AttrValue> GetAttr(const char  *attrName) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attrname | Input | Attribute name. |

## Return Value

The obtained pointer of the AttrValue type.

## Exceptions

None.

## Constraints

None.
