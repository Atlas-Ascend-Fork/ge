# aclopSetAttrListDataType

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:53:30.310Z pushedAt=2026-10-29T03:55:36.768Z nl=2 -->

## Product Support

<!-- npu="950" id442 -->
- Ascend 950PR&950DT products: Supported
<!-- end id442 -->
<!-- npu="A3" id443 -->
- Atlas A3 products: Supported
<!-- end id443 -->
<!-- npu="910b" id444 -->
- Atlas A2 products: Supported
<!-- end id444 -->
<!-- npu="310b" id445 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id445 -->
<!-- npu="310p" id446 -->
- Atlas inference products: Supported
<!-- end id446 -->
<!-- npu="910" id447 -->
- Atlas training products: Supported
<!-- end id447 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrListDataType_res.md#id1 -->

## Feature Description

Sets the data type of a specified attribute list.

## Prototype

```c
aclError aclopSetAttrListDataType(aclopAttr *attr, const char *attrName, int numValues, const aclDataType values[])
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| numValues | Input | Number of attribute values. |
| values | Input | Array of attribute values. See aclDataType for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
