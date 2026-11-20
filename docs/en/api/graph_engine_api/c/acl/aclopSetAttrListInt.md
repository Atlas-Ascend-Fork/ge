# aclopSetAttrListInt

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:09:52.323Z pushedAt=2026-11-18T06:13:26.950Z nl=2 -->

## Product Support

<!-- npu="950" id1091 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1091 -->
<!-- npu="A3" id1092 -->
- Atlas A3 products: Supported
<!-- end id1092 -->
<!-- npu="910b" id1093 -->
- Atlas A2 products: Supported
<!-- end id1093 -->
<!-- npu="310b" id1094 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1094 -->
<!-- npu="310p" id1095 -->
- Atlas inference products: Supported
<!-- end id1095 -->
<!-- npu="910" id1096 -->
- Atlas training products: Supported
<!-- end id1096 -->
<!-- npu="IPV350" id1097 -->
- IPV350: not supported
<!-- end id1097 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrListInt_res.md#id1 -->

## Function Description

Sets the attribute value of an int64\_t type list.

## Prototype

```c
aclError aclopSetAttrListInt(aclopAttr *attr, const char *attrName, int numValues, const int64_t *values)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| numValues | Input | Number of attribute values. |
| values | Input | Array of attribute values. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
