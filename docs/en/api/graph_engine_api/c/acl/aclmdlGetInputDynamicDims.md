# aclmdlGetInputDynamicDims

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:16:46.331Z pushedAt=2026-10-29T03:17:59.887Z nl=2 -->

## Product Support

<!-- npu="950" id734 -->
- Ascend 950PR&950DT products: Supported
<!-- end id734 -->
<!-- npu="A3" id735 -->
- Atlas A3 products: Supported
<!-- end id735 -->
<!-- npu="910b" id736 -->
- Atlas A2 products: Supported
<!-- end id736 -->
<!-- npu="310b" id737 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id737 -->
<!-- npu="310p" id738 -->
- Atlas inference products: Supported
<!-- end id738 -->
<!-- npu="910" id739 -->
- Atlas training products: Supported
<!-- end id739 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputDynamicDims_res.md#id1 -->

## Feature Description

Obtains the dynamic dimension information supported by the model input based on the model description.

## Prototype

```c
aclError aclmdlGetInputDynamicDims(const aclmdlDesc *modelDesc, size_t index, aclmdlIODims *dims, size_t gearCount)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| index | Input | Reserved parameter, which is not used currently. Set it to -1. |
| dims | Output | Pointer to the input dynamic dimension information. For the type definition, see [aclmdlIODims](aclmdlIODims.md).<br>The dims parameter is an array, and each element in the array points to the [aclmdlIODims](aclmdlIODims.md) structure. The dims parameter in the [aclmdlIODims](aclmdlIODims.md) structure is also an array, and each element in this array corresponds to a specific value in each gear.<br>For example:<br>aclmdlIODims dims[gearCount];<br>aclmdlGetInputDynamicDims(model.modelDesc, -1, dims, gearCount); |
| gearCount | Input | Number of dynamic dimension gears supported by the model. Obtain it by calling [aclmdlGetInputDynamicGearCount](aclmdlGetInputDynamicGearCount.md) first. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

You can call this API to obtain dynamic dimension information only after the dynamic dimension profiling information is configured during model compilation.

For example, a model has three inputs: data\(1, 1, 40, -1\), label\(1, -1\), mask\(-1, -1\), where -1 indicates a dynamic variable. During model conversion, the configuration example of the dynamic\_dims parameter is --dynamic\_dims="20,20,1,1;40,40,2,2;80,60,4,4". The dynamic dimension information obtained through this API is as follows (the name in the **[aclmdlIODims](aclmdlIODims.md)** structure is not used for now):

- Profile 0:
    - dimCount in the **[aclmdlIODims](aclmdlIODims.md)** structure: 8, which indicates the sum of the number of dimensions of all input tensors.
    - dims in the **[aclmdlIODims](aclmdlIODims.md)** structure: "1,1,40,20,1,20,1,1", which indicates data\(1,1,40,20\)+label\(1,20\)+mask\(1,1\)

- Profile 1:
    - dimCount in the **[aclmdlIODims](aclmdlIODims.md)** structure: 8, which indicates the sum of the number of dimensions of all input tensors.
    - dims in the **[aclmdlIODims](aclmdlIODims.md)** structure: "1,1,40,40,1,40,2,2", which indicates data\(1,1,40,40\)+label\(1,40\)+mask\(2,2\)

- Profile 2:
    - In the **[aclmdlIODims](aclmdlIODims.md)** structure, dimCount: 8, which indicates the sum of the number of dimensions of all input tensors.
    - In the **[aclmdlIODims](aclmdlIODims.md)** structure, dims: "1,1,40,80,1,60,4,4", which indicates data\(1,1,40,80\)+label\(1,60\)+mask\(4,4\)
