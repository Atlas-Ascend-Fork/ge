# aclopSetAttrListFloat

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:53:52.624Z pushedAt=2026-10-29T03:55:43.530Z nl=2 -->

## Product Support

<!-- npu="950" id1070 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1070 -->
<!-- npu="A3" id1071 -->
- Atlas A3 products: Supported
<!-- end id1071 -->
<!-- npu="910b" id1072 -->
- Atlas A2 products: Supported
<!-- end id1072 -->
<!-- npu="310b" id1073 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1073 -->
<!-- npu="310p" id1074 -->
- Atlas inference products: Supported
<!-- end id1074 -->
<!-- npu="910" id1075 -->
- Atlas training products: Supported
<!-- end id1075 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrListFloat_res.md#id1 -->

## Feature Description

Sets the value of a float-type list attribute.

## Prototype

```c
aclError aclopSetAttrListFloat(aclopAttr *attr, const char *attrName, int numValues, const float *values)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| numValues | Input | Number of attribute values. |
| values | Input | Array of attribute values. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
