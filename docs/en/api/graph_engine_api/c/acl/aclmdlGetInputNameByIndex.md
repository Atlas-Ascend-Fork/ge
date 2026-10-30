# aclmdlGetInputNameByIndex

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:19:09.458Z pushedAt=2026-10-29T03:18:27.986Z nl=2 -->

## Product Support

<!-- npu="950" id316 -->
- Ascend 950PR&950DT products: Supported
<!-- end id316 -->
<!-- npu="A3" id317 -->
- Atlas A3 products: Supported
<!-- end id317 -->
<!-- npu="910b" id318 -->
- Atlas A2 products: Supported
<!-- end id318 -->
<!-- npu="310b" id319 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id319 -->
<!-- npu="310p" id320 -->
- Atlas inference products: Supported
<!-- end id320 -->
<!-- npu="910" id321 -->
- Atlas training products: Supported
<!-- end id321 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputNameByIndex_res.md#id1 -->

## Feature Description

Obtains the name of a specified input in the model based on the model description.

## Prototype

```c
const char *aclmdlGetInputNameByIndex(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the input whose name is to be obtained. The index value starts from 0. |

## Return Value

Returns the name of the specified input.
