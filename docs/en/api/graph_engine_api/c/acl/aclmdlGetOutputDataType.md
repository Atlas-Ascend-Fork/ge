# aclmdlGetOutputDataType

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:22:00.118Z pushedAt=2026-10-29T03:19:29.733Z nl=2 -->

## Product Support

<!-- npu="950" id608 -->
- Ascend 950PR&950DT products: Supported
<!-- end id608 -->
<!-- npu="A3" id609 -->
- Atlas A3 products: Supported
<!-- end id609 -->
<!-- npu="910b" id610 -->
- Atlas A2 products: Supported
<!-- end id610 -->
<!-- npu="310b" id611 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id611 -->
<!-- npu="310p" id612 -->
- Atlas inference products: Supported
<!-- end id612 -->
<!-- npu="910" id613 -->
- Atlas training products: Supported
<!-- end id613 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputDataType_res.md#id1 -->

## Feature Description

Obtains the data type of a specified output in a model based on the model description.

## Prototype

```c
aclDataType aclmdlGetOutputDataType(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| parameter name | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Index of the output whose data type is to be obtained. The index value starts from 0. |

## Return Value

Returns the data type of the specified output, which is of the aclDataType type.
