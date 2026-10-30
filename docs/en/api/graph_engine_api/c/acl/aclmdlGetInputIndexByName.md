# aclmdlGetInputIndexByName

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:18:35.181Z pushedAt=2026-10-29T03:18:21.908Z nl=2 -->

## Product Support

<!-- npu="950" id867 -->
- Ascend 950PR&950DT products: Supported
<!-- end id867 -->
<!-- npu="A3" id868 -->
- Atlas A3 products: Supported
<!-- end id868 -->
<!-- npu="910b" id869 -->
- Atlas A2 products: Supported
<!-- end id869 -->
<!-- npu="310b" id870 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id870 -->
<!-- npu="310p" id871 -->
- Atlas inference products: Supported
<!-- end id871 -->
<!-- npu="910" id872 -->
- Atlas training products: Supported
<!-- end id872 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputIndexByName_res.md#id1 -->

## Feature Description

Obtains the index of an input based on its name in the model.

## Prototype

```c
aclError aclmdlGetInputIndexByName(const aclmdlDesc *modelDesc, const char *name, size_t *index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| name | Input | Pointer to the input name. |
| index | Output | Pointer to the input index. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
