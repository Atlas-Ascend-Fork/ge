# aclmdlGetInputDimsRange

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:15:48.942Z pushedAt=2026-10-29T03:14:41.763Z nl=2 -->

## Product Support

<!-- npu="950" id330 -->
- Ascend 950PR&950DT products: Supported
<!-- end id330 -->
<!-- npu="A3" id331 -->
- Atlas A3 products: Supported
<!-- end id331 -->
<!-- npu="910b" id332 -->
- Atlas A2 products: Supported
<!-- end id332 -->
<!-- npu="310b" id333 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id333 -->
<!-- npu="310p" id334 -->
- Atlas inference products: Supported
<!-- end id334 -->
<!-- npu="910" id335 -->
- Atlas training products: Supported
<!-- end id335 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputDimsRange_res.md#id1 -->

## Feature Description

Obtains the dimension range of the input tensor of a model based on the model description.

## Prototype

```c
aclError aclmdlGetInputDimsRange(const aclmdlDesc *modelDesc, size_t index, aclmdlIODimsRange *dimsRange)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the input whose dimension information is to be obtained. The value of index starts from 0. |
| dimsRange | Output | Pointer to the input dimension information. For the type definition, see [aclmdlIODimsRange](aclmdlIODimsRange.md).<br><br>  - In the dynamic Shape scenario with a configured Shape range, the Shape range is returned. The upper and lower limits of dynamic dimensions are different, while those of static dimensions are the same. For example, if the format of the input tensor is NCHW and dims is [1,3,10~224,10~300], where the N and C dimensions are fixed values and the H and W dimensions are dynamic ranges, this API returns aclmdlIODimsRange.rangeCount of 4 and four elements in the aclmdlIODimsRange.range array, with the values {1,1}, {3,3}, {10,224}, and {10,300}, respectively.<br>  - In the dynamic Shape scenario with configured profiles, this API is not supported, and aclmdlIODimsRange.rangeCount returns 0.<br>  - In the static Shape scenario, the Shape range is returned, but the upper and lower limits of each dimension are the same. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
