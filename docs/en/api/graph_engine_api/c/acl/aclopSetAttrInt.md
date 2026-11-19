# aclopSetAttrInt

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:07:57.408Z pushedAt=2026-11-18T03:43:13.943Z nl=2 -->

## Product Support

<!-- npu="950" id846 -->
- Ascend 950PR&950DT products: Supported
<!-- end id846 -->
<!-- npu="A3" id847 -->
- Atlas A3 products: Supported
<!-- end id847 -->
<!-- npu="910b" id848 -->
- Atlas A2 products: Supported
<!-- end id848 -->
<!-- npu="310b" id849 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id849 -->
<!-- npu="310p" id850 -->
- Atlas inference products: Supported
<!-- end id850 -->
<!-- npu="910" id851 -->
- Atlas training products: Supported
<!-- end id851 -->
<!-- npu="IPV350" id852 -->
- IPV350: not supported
<!-- end id852 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrInt_res.md#id1 -->

## Function Description

Sets the attribute value of an int64\_t type scalar.

## Prototype

```c
aclError aclopSetAttrInt(aclopAttr *attr, const char *attrName, int64_t attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| attrValue | Input | Attribute value. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
