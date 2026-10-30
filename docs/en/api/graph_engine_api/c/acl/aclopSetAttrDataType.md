# aclopSetAttrDataType

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:51:54.085Z pushedAt=2026-10-29T03:53:57.791Z nl=2 -->

## Product Support

<!-- npu="950" id78 -->
- Ascend 950PR&950DT products: Supported
<!-- end id78 -->
<!-- npu="A3" id79 -->
- Atlas A3 products: Supported
<!-- end id79 -->
<!-- npu="910b" id80 -->
- Atlas A2 products: Supported
<!-- end id80 -->
<!-- npu="310b" id81 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id81 -->
<!-- npu="310p" id82 -->
- Atlas inference products: Supported
<!-- end id82 -->
<!-- npu="910" id83 -->
- Atlas training products: Supported
<!-- end id83 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrDataType_res.md#id1 -->

## Feature Description

Sets the data type of a specified attribute.

## Prototype

```c
aclError aclopSetAttrDataType(aclopAttr *attr, const char *attrName, aclDataType attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| attrValue | Input | Attribute value. See aclDataType for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
