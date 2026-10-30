# aclmdlGetDatasetNumBuffers

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:10:31.576Z pushedAt=2026-10-29T03:11:04.431Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDatasetNumBuffers_res.md#id1 -->

## Feature Description

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
