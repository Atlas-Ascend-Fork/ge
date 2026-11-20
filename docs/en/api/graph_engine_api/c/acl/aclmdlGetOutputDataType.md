# aclmdlGetOutputDataType

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:28:45.169Z pushedAt=2026-11-18T03:32:37.667Z nl=2 -->

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
<!-- npu="IPV350" id614 -->
- IPV350: support
<!-- end id614 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputDataType_res.md#id1 -->

## Function Description

Gets the data type of the specified output in the model based on the om/om2 model description.

## Prototype

```c
aclDataType aclmdlGetOutputDataType(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Index of the output whose data type is to be obtained. The index value starts from 0. |

## Return Value

Returns the data type of the specified output, which is of the aclDataType type.
