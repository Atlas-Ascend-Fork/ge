# aclopSetAttrFloat

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:06:59.730Z pushedAt=2026-11-18T03:43:08.430Z nl=2 -->

## Product Support

<!-- npu="950" id218 -->
- Ascend 950PR&950DT products: Supported
<!-- end id218 -->
<!-- npu="A3" id219 -->
- Atlas A3 products: Supported
<!-- end id219 -->
<!-- npu="910b" id220 -->
- Atlas A2 products: Supported
<!-- end id220 -->
<!-- npu="310b" id221 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id221 -->
<!-- npu="310p" id222 -->
- Atlas inference products: Supported
<!-- end id222 -->
<!-- npu="910" id223 -->
- Atlas training products: Supported
<!-- end id223 -->
<!-- npu="IPV350" id224 -->
- IPV350: not supported
<!-- end id224 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrFloat_res.md#id1 -->

## Function Description

Sets the attribute value of a float scalar.

## Prototype

```c
aclError aclopSetAttrFloat(aclopAttr *attr, const char *attrName, float attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| attrValue | Input | Attribute value. |

## Return Value

Returns 0 on success; otherwise, returns a failure value.
