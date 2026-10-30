# aclopSetAttrListString

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:55:10.893Z pushedAt=2026-10-29T03:57:23.410Z nl=2 -->

## Product Support

<!-- npu="950" id463 -->
- Ascend 950PR&950DT products: Supported
<!-- end id463 -->
<!-- npu="A3" id464 -->
- Atlas A3 products: Supported
<!-- end id464 -->
<!-- npu="910b" id465 -->
- Atlas A2 products: Supported
<!-- end id465 -->
<!-- npu="310b" id466 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id466 -->
<!-- npu="310p" id467 -->
- Atlas inference products: Supported
<!-- end id467 -->
<!-- npu="910" id468 -->
- Atlas training products: Supported
<!-- end id468 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrListString_res.md#id1 -->

## Feature Description

Sets the attribute value of a string list.

## Prototype

```c
aclError aclopSetAttrListString(aclopAttr *attr, const char *attrName, int numValues, const char **values)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| numValues | Input | Number of attribute values. |
| values | Input | Pointer to the array of attribute values. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
