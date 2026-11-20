# aclopSetAttrListString

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:10:53.941Z pushedAt=2026-11-18T06:13:43.534Z nl=2 -->

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
<!-- npu="IPV350" id469 -->
- IPV350: not supported
<!-- end id469 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrListString_res.md#id1 -->

## Function Description

Sets the attribute value of a string list.

## Prototype

```c
aclError aclopSetAttrListString(aclopAttr *attr, const char *attrName, int numValues, const char **values)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| numValues | Input | Number of attribute values. |
| values | Input | Pointer to the array of attribute values. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
