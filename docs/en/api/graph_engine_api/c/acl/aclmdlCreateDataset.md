# aclmdlCreateDataset

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:04:46.252Z pushedAt=2026-11-18T03:19:24.543Z nl=2 -->

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
<!-- npu="IPV350" id901 -->
- IPV350: support
<!-- end id901 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateDataset_res.md#id1 -->

## Function Description

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
