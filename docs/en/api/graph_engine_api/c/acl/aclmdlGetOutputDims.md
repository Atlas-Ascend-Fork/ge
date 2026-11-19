# aclmdlGetOutputDims

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:29:31.377Z pushedAt=2026-11-18T03:32:47.490Z nl=2 -->

## Product Support

<!-- npu="950" id546 -->
- Ascend 950PR&950DT products: Supported
<!-- end id546 -->
<!-- npu="A3" id547 -->
- Atlas A3 products: Supported
<!-- end id547 -->
<!-- npu="910b" id548 -->
- Atlas A2 products: Supported
<!-- end id548 -->
<!-- npu="310b" id549 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id549 -->
<!-- npu="310p" id550 -->
- Atlas inference products: Supported
<!-- end id550 -->
<!-- npu="910" id551 -->
- Atlas training products: Supported
<!-- end id551 -->
<!-- npu="IPV350" id552 -->
- IPV350: support
<!-- end id552 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputDims_res.md#id1 -->

## Function Description

Obtains the dimension information of a specified model output tensor based on the om/om2 model description.

In fixed shape scenarios, this API obtains the dimension information of a specified model output tensor.

In dynamic shape scenarios (dynamic batch or dynamic resolution), this API obtains the dimension information of the maximum profile.

## Prototype

```c
aclError aclmdlGetOutputDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the output whose Dims are to be obtained. The value of index starts from 0. |
| dims | Output | Pointer to the output dimension information. For the type definition, see [aclmdlIODims](aclmdlIODims.md).<br>If the tensor name length is greater than 127, when outputting dims.name, the system converts the tensor name to the "acl_modelId_${id}_output_${index} _${random string}  " format. If the converted tensor name conflicts with an existing tensor name in the model, "_${random string}  " is appended to the end of the converted name; otherwise, no random string is appended. A mapping is established between the converted name and the original name. You can call [aclmdlGetTensorRealName](aclmdlGetTensorRealName.md) and pass the converted name to obtain the original name. If you pass the original name to this API, the original name is still returned. If the tensor name length is less than or equal to 127, the tensor name is output as dims.name. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
