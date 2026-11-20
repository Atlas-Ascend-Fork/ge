# aclmdlGetInputSizeByIndex

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:26:51.708Z pushedAt=2026-11-18T03:32:16.806Z nl=2 -->

## Product Support

<!-- npu="950" id498 -->
- Ascend 950PR&950DT products: Supported
<!-- end id498 -->
<!-- npu="A3" id499 -->
- Atlas A3 products: Supported
<!-- end id499 -->
<!-- npu="910b" id500 -->
- Atlas A2 products: Supported
<!-- end id500 -->
<!-- npu="310b" id501 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id501 -->
<!-- npu="310p" id502 -->
- Atlas inference products: Supported
<!-- end id502 -->
<!-- npu="910" id503 -->
- Atlas training products: Supported
<!-- end id503 -->
<!-- npu="IPV350" id504 -->
- IPV350: support
<!-- end id504 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetInputSizeByIndex_res.md#id1 -->

## Function Description

Obtains the size of a specified input according to the om/om2 model description, in bytes.

## Prototype

```c
size_t aclmdlGetInputSizeByIndex(aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the input whose size is to be obtained. The value of index starts from 0. |

## Return Value

For dynamic batch and dynamic resolution (width and height) scenarios, returns the input size corresponding to the maximum profile; for static scenarios, returns the size of the specified input. The unit is Byte.

## Constraints

If the input shape of the model is dynamic and a dimension value is -1 (indicating that this dimension can take any value of \>=1), the size obtained through this API is 0. You need to allocate memory based on the actual memory size occupied by the data.
