# aclmdlGetAttribute

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:09:41.140Z pushedAt=2026-10-29T03:07:05.118Z nl=2 -->

## Product Support

<!-- npu="950" id881 -->
- Ascend 950PR&950DT products: Supported
<!-- end id881 -->
<!-- npu="A3" id882 -->
- Atlas A3 products: Supported
<!-- end id882 -->
<!-- npu="910b" id883 -->
- Atlas A2 products: Supported
<!-- end id883 -->
<!-- npu="310b" id884 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id884 -->
<!-- npu="310p" id885 -->
- Atlas inference products: Supported
<!-- end id885 -->
<!-- npu="910" id886 -->
- Atlas training products: Supported
<!-- end id886 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetAttribute_res.md#id1 -->

## Feature Description

Obtains the model attribute value.

## Prototype

```c
aclError aclmdlGetAttribute(uint32_t modelId, aclmdlAttr attr, aclmdlAttrValue_t *attrValue)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the model.<br>After a model loading API (for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) or [aclmdlLoadFromMem](aclmdlLoadFromMem.md)) is successfully called, a model ID is returned and used as the input of this API. |
| attr | Input | Attribute to be set. For the type definition, see also [aclmdlAttr](aclmdlAttr.md). |
| attrValue | Output | Pointer to the attribute value, which is the value of the attribute corresponding to attr. For the type definition, see also [aclmdlAttrValue_t](aclmdlAttrValue_t.md). |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See also [aclError](aclError.md).
