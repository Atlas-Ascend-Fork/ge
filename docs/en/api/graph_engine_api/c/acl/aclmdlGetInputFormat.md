# aclmdlGetInputFormat

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:24:44.040Z pushedAt=2026-11-18T03:32:01.989Z nl=2 -->

## Product Support

<!-- npu="950" id456 -->
- Ascend 950PR&950DT products: Supported
<!-- end id456 -->
<!-- npu="A3" id457 -->
- Atlas A3 products: Supported
<!-- end id457 -->
<!-- npu="910b" id458 -->
- Atlas A2 products: Supported
<!-- end id458 -->
<!-- npu="310b" id459 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id459 -->
<!-- npu="310p" id460 -->
- Atlas inference products: Supported
<!-- end id460 -->
<!-- npu="910" id461 -->
- Atlas training products: Supported
<!-- end id461 -->
<!-- npu="IPV350" id462 -->
- IPV350: support
<!-- end id462 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputFormat_res.md#id1 -->

## Function Description

Obtains the Format of a specified input in the model based on the om/om2 model description.

## Prototype

```c
aclFormat aclmdlGetInputFormat(const aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the input whose Format is to be obtained. The value of index starts from 0. |

## Return Value

Returns the Format of the specified input, of the aclFormat type.
