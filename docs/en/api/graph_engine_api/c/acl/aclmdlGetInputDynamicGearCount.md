# aclmdlGetInputDynamicGearCount

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:17:18.855Z pushedAt=2026-10-29T03:18:06.216Z nl=2 -->

## Product Support

<!-- npu="950" id1063 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1063 -->
<!-- npu="A3" id1064 -->
- Atlas A3 products: Supported
<!-- end id1064 -->
<!-- npu="910b" id1065 -->
- Atlas A2 products: Supported
<!-- end id1065 -->
<!-- npu="310b" id1066 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1066 -->
<!-- npu="310p" id1067 -->
- Atlas inference products: Supported
<!-- end id1067 -->
<!-- npu="910" id1068 -->
- Atlas training products: Supported
<!-- end id1068 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputDynamicGearCount_res.md#id1 -->

## Feature Description

Obtains the number of dynamic dimension gears supported by the model input based on the model description.

## Prototype

```c
aclError aclmdlGetInputDynamicGearCount(const aclmdlDesc *modelDesc, size_t index, size_t *gearCount)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Reserved parameter, which is not used currently. Set it to -1. |
| gearCount | Output | Pointer to the number of dynamic dimension gears.<br>For example, if the model input tensor is 4-dimensional and the gears configured through the --dynamic_dims parameter during model conversion are "1,3,224,224;2,3,224,224;1,3,256,256", the number of dynamic dimension gears obtained through this API is 3. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

If no dynamic dimension gears are configured during model compilation, the number of dynamic dimension gears obtained through this API is 0.
