# aclmdlCreateDataset

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:00:52.648Z pushedAt=2026-10-29T03:00:05.675Z nl=2 -->

## Product Support

<!-- npu="950" id895 -->
- Ascend 950PR&950DT products: Supported
<!-- end id895 -->
<!-- npu="A3" id896 -->
- Atlas A3 products: Supported
<!-- end id896 -->
<!-- npu="910b" id897 -->
- Atlas A2 products: Supported
<!-- end id897 -->
<!-- npu="310b" id898 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id898 -->
<!-- npu="310p" id899 -->
- Atlas inference products: Supported
<!-- end id899 -->
<!-- npu="910" id900 -->
- Atlas training products: Supported
<!-- end id900 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateDataset_res.md#id1 -->

## Feature Description

Creates data of the aclmdlDataset type, which describes the input and output data for model inference. A model may have multiple inputs and multiple outputs, and the memory address and memory size of each input/output are described by data of the aclDataBuffer type.

To destroy data of the aclmdlDataset type, see [aclmdlDestroyDataset](aclmdlDestroyDataset.md).

## Prototype

```c
aclmdlDataset *aclmdlCreateDataset()
```

## Parameters

None

## Return Value

Returns a pointer of the aclmdlDataset type.
