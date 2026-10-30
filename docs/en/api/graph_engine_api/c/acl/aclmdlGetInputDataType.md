# aclmdlGetInputDataType

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:15:33.170Z pushedAt=2026-10-29T03:14:07.205Z nl=2 -->

## Product Support

<!-- npu="950" id923 -->
- Ascend 950PR&950DT products: Supported
<!-- end id923 -->
<!-- npu="A3" id924 -->
- Atlas A3 products: Supported
<!-- end id924 -->
<!-- npu="910b" id925 -->
- Atlas A2 products: Supported
<!-- end id925 -->
<!-- npu="310b" id926 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id926 -->
<!-- npu="310p" id927 -->
- Atlas inference products: Supported
<!-- end id927 -->
<!-- npu="910" id928 -->
- Atlas training products: Supported
<!-- end id928 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputDataType_res.md#id1 -->

## Feature Description

Obtains the data type of a specified input in the model based on the model description.

## Prototype

```c
aclDataType aclmdlGetInputDataType(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Index of the input whose data type is to be obtained. The index value starts from 0. |

## Return Value

Returns the data type of the specified input, which is of the aclDataType type.
