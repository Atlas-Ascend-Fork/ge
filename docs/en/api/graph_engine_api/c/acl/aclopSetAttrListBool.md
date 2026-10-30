# aclopSetAttrListBool

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:53:13.684Z pushedAt=2026-10-29T03:55:29.561Z nl=2 -->

## Product Support

<!-- npu="950" id106 -->
- Ascend 950PR&950DT products: Supported
<!-- end id106 -->
<!-- npu="A3" id107 -->
- Atlas A3 products: Supported
<!-- end id107 -->
<!-- npu="910b" id108 -->
- Atlas A2 products: Supported
<!-- end id108 -->
<!-- npu="310b" id109 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id109 -->
<!-- npu="310p" id110 -->
- Atlas inference products: Supported
<!-- end id110 -->
<!-- npu="910" id111 -->
- Atlas training products: Supported
<!-- end id111 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrListBool_res.md#id1 -->

## Feature Description

Sets the attribute value of a bool list.

## Prototype

```c
aclError aclopSetAttrListBool(aclopAttr *attr, const char *attrName, int numValues, const uint8_t *values)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| numValues | Input | Number of attribute values. |
| values | Input | Array of attribute values. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
