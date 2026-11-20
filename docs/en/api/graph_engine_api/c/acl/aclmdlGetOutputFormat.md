# aclmdlGetOutputFormat

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:29:53.572Z pushedAt=2026-11-18T03:32:54.362Z nl=2 -->

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
<!-- npu="IPV350" id245 -->
- IPV350: support
<!-- end id245 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputFormat_res.md#id1 -->

## Function Description

Obtains the Format of a specified output in a model based on the om/om2 model description information.

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
