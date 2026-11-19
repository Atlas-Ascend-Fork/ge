# aclmdlGetInputDataType

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:21:26.380Z pushedAt=2026-11-18T03:31:13.576Z nl=2 -->

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
<!-- npu="IPV350" id929 -->
- IPV350: support
<!-- end id929 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputDataType_res.md#id1 -->

## Function Description

Gets the data type of the specified input in the model based on the om/om2 model description.

## Prototype

```c
aclDataType aclmdlGetInputDataType(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Index of the input whose data type is to be obtained. The index value starts from 0. |

## Return Value

Returns the data type of the specified input, which is of the aclDataType type.
