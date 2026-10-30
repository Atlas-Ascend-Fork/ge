# aclopSetAttrString

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:55:49.821Z pushedAt=2026-10-29T03:57:45.700Z nl=2 -->

## Product Support

<!-- npu="950" id567 -->
- Ascend 950PR&950DT products: Supported
<!-- end id567 -->
<!-- npu="A3" id568 -->
- Atlas A3 products: Supported
<!-- end id568 -->
<!-- npu="910b" id569 -->
- Atlas A2 products: Supported
<!-- end id569 -->
<!-- npu="310b" id570 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id570 -->
<!-- npu="310p" id571 -->
- Atlas inference products: Supported
<!-- end id571 -->
<!-- npu="910" id572 -->
- Atlas training products: Supported
<!-- end id572 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrString_res.md#id1 -->

## Feature Description

Sets the attribute value of a string-type scalar.

## Prototype

```c
aclError aclopSetAttrString(aclopAttr *attr, const char *attrName, const char *attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| attrName | Input | Pointer to the attribute name. |
| attrValue | Input | Pointer to the attribute value. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
