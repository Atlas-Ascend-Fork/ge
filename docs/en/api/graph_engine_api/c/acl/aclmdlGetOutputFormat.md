# aclmdlGetOutputFormat

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:21:51.956Z pushedAt=2026-10-29T03:21:36.589Z nl=2 -->

## Product Support

<!-- npu="950" id239 -->
- Ascend 950PR&950DT products: Supported
<!-- end id239 -->
<!-- npu="A3" id240 -->
- Atlas A3 products: Supported
<!-- end id240 -->
<!-- npu="910b" id241 -->
- Atlas A2 products: Supported
<!-- end id241 -->
<!-- npu="310b" id242 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id242 -->
<!-- npu="310p" id243 -->
- Atlas inference products: Supported
<!-- end id243 -->
<!-- npu="910" id244 -->
- Atlas training products: Supported
<!-- end id244 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputFormat_res.md#id1 -->

## Feature Description

Obtains the Format of a specified output in a model based on the model description information.

## Prototype

```c
aclFormat aclmdlGetOutputFormat(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the output whose Format is to be obtained. The value of index starts from 0. |

## Return Value

Returns the Format of the specified output, of type aclFormat.
