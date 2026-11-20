# aclmdlGetTensorRealName

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:32:10.929Z pushedAt=2026-11-18T03:33:30.151Z nl=2 -->

## Product Support

<!-- npu="950" id1021 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1021 -->
<!-- npu="A3" id1022 -->
- Atlas A3 products: Supported
<!-- end id1022 -->
<!-- npu="910b" id1023 -->
- Atlas A2 products: Supported
<!-- end id1023 -->
<!-- npu="310b" id1024 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1024 -->
<!-- npu="310p" id1025 -->
- Atlas inference products: Supported
<!-- end id1025 -->
<!-- npu="910" id1026 -->
- Atlas training products: Supported
<!-- end id1026 -->
<!-- npu="IPV350" id1027 -->
- IPV350: not supported
<!-- end id1027 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetTensorRealName_res.md#id1 -->

## Function Description

Obtains the real name of a tensor by the specified name.

The aclmdlGetTensorRealName API must be used together with the [aclmdlGetInputDims](aclmdlGetInputDims.md)/[aclmdlGetInputDimsV2](aclmdlGetInputDimsV2.md)/[aclmdlGetOutputDims](aclmdlGetOutputDims.md)/[aclmdlGetCurOutputDims](aclmdlGetCurOutputDims.md) APIs.

## Prototype

```c
const char *aclmdlGetTensorRealName(const aclmdlDesc *modelDesc, const char *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| name | Input | Pointer to the name, used to obtain the real name of the tensor based on this name. |

## Return Value

Returns a pointer to the real name of the tensor. The lifetime of this pointer is the same as that of modelDesc. If the modelDesc resource is destroyed, the content pointed to by this pointer is automatically destroyed as well.

Returns nullptr if modelDesc or name is null.
