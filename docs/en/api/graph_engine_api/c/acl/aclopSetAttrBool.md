# aclopSetAttrBool

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:05:39.509Z pushedAt=2026-11-18T03:42:58.592Z nl=2 -->

## Product Support

<!-- npu="950" id85 -->
- Ascend 950PR&950DT products: Supported
<!-- end id85 -->
<!-- npu="A3" id86 -->
- Atlas A3 products: Supported
<!-- end id86 -->
<!-- npu="910b" id87 -->
- Atlas A2 products: Supported
<!-- end id87 -->
<!-- npu="310b" id88 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id88 -->
<!-- npu="310p" id89 -->
- Atlas inference products: Supported
<!-- end id89 -->
<!-- npu="910" id90 -->
- Atlas training products: Supported
<!-- end id90 -->
<!-- npu="IPV350" id91 -->
- IPV350: not supported
<!-- end id91 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrBool_res.md#id1 -->

## Function Description

Sets the attribute value of a bool scalar.

## Prototype

```c
aclError aclopSetAttrBool(aclopAttr *attr, const char *attrName, uint8_t attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| attrValue | Input | Attribute value. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
