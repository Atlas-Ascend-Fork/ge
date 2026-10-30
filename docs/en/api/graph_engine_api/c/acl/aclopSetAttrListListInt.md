# aclopSetAttrListListInt

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:54:56.937Z pushedAt=2026-10-29T03:57:15.059Z nl=2 -->

## Product Support

<!-- npu="950" id176 -->
- Ascend 950PR&950DT products: Supported
<!-- end id176 -->
<!-- npu="A3" id177 -->
- Atlas A3 products: Supported
<!-- end id177 -->
<!-- npu="910b" id178 -->
- Atlas A2 products: Supported
<!-- end id178 -->
<!-- npu="310b" id179 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id179 -->
<!-- npu="310p" id180 -->
- Atlas inference products: Supported
<!-- end id180 -->
<!-- npu="910" id181 -->
- Atlas training products: Supported
<!-- end id181 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrListListInt_res.md#id1 -->

## Feature Description

Sets the attribute value of an int64\_t type list.

## Prototype

```c
aclError aclopSetAttrListListInt(aclopAttr *attr,
const char *attrName,
int numLists,
const int *numValues,
const int64_t *const values[])
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| numLists | Input | Number of lists. |
| numValues | Input | Number of attribute values in a list.<br>numValues is an array, where each element indicates the number of attribute values in each list. |
| values | Input | Pointer array of attribute values in the list. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
