# aclmdlDestroyDataset

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:04:32.835Z pushedAt=2026-10-29T03:01:20.857Z nl=2 -->

## Product Support

<!-- npu="950" id197 -->
- Ascend 950PR&950DT products: Supported
<!-- end id197 -->
<!-- npu="A3" id198 -->
- Atlas A3 products: Supported
<!-- end id198 -->
<!-- npu="910b" id199 -->
- Atlas A2 products: Supported
<!-- end id199 -->
<!-- npu="310b" id200 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id200 -->
<!-- npu="310p" id201 -->
- Atlas inference products: Supported
<!-- end id201 -->
<!-- npu="910" id202 -->
- Atlas training products: Supported
<!-- end id202 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlDestroyDataset_res.md#id1 -->

## Feature Description

Destroys the aclmdlDataset data created by the [aclmdlCreateDataset](aclmdlCreateDataset.md) API.

## Prototype

```c
aclError aclmdlDestroyDataset(const aclmdlDataset *dataset)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataset | Input | Pointer to the aclmdlDataset object to be destroyed. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
