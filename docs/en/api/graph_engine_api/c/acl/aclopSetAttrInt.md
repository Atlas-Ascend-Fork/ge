# aclopSetAttrInt

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:52:47.113Z pushedAt=2026-10-29T03:54:36.999Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetAttrInt_res.md#id1 -->

## Feature Description

Sets the attribute value of an int64\_t type scalar.

## Prototype

```c
aclError aclopSetAttrInt(aclopAttr *attr, const char *attrName, int64_t attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr | Output | Pointer to the aclopAttr type.<br>Call [aclopCreateAttr](aclopCreateAttr.md) to create the aclopAttr type beforehand. |
| attrName | Input | Pointer to the attribute name. |
| attrValue | Input | Attribute value. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
