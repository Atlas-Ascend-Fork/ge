# aclopSetAttrDataType

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:06:34.371Z pushedAt=2026-11-18T03:43:04.695Z nl=2 -->

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
<!-- npu="IPV350" id84 -->
- IPV350: not supported
<!-- end id84 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrDataType_res.md#id1 -->

## Function Description

Sets the data type of a specified attribute.

## Prototype

```c
aclError aclopSetAttrDataType(aclopAttr *attr, const char *attrName, aclDataType attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| attrValue | Input | Attribute value. See aclDataType for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
