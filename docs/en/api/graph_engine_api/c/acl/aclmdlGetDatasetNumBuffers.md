# aclmdlGetDatasetNumBuffers

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:17:16.924Z pushedAt=2026-11-18T03:30:11.878Z nl=2 -->

## Product Support

<!-- npu="950" id713 -->
- Ascend 950PR&950DT products: Supported
<!-- end id713 -->
<!-- npu="A3" id714 -->
- Atlas A3 products: Supported
<!-- end id714 -->
<!-- npu="910b" id715 -->
- Atlas A2 products: Supported
<!-- end id715 -->
<!-- npu="310b" id716 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id716 -->
<!-- npu="310p" id717 -->
- Atlas inference products: Supported
<!-- end id717 -->
<!-- npu="910" id718 -->
- Atlas training products: Supported
<!-- end id718 -->
<!-- npu="IPV350" id719 -->
- IPV350: support
<!-- end id719 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDatasetNumBuffers_res.md#id1 -->

## Function Description

Obtains the number of aclDataBuffer objects in aclmdlDataset.

## Prototype

```c
size_t aclmdlGetDatasetNumBuffers(const aclmdlDataset *dataset)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataset | Input | Pointer to the aclmdlDataset type.<br>Call [aclmdlCreateDataset](aclmdlCreateDataset.md) in advance to create data of the aclmdlDataset type. |

## Return Value

Returns the number of aclDataBuffer objects.
