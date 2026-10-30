# aclmdlGetFirstAippInfo

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:14:02.067Z pushedAt=2026-10-29T03:13:59.911Z nl=2 -->

## Product Support

<!-- npu="950" id930 -->
- Ascend 950PR&950DT products: Supported
<!-- end id930 -->
<!-- npu="A3" id931 -->
- Atlas A3 products: Supported
<!-- end id931 -->
<!-- npu="910b" id932 -->
- Atlas A2 products: Supported
<!-- end id932 -->
<!-- npu="310b" id933 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id933 -->
<!-- npu="310p" id934 -->
- Atlas inference products: Supported
<!-- end id934 -->
<!-- npu="910" id935 -->
- Atlas training products: Supported
<!-- end id935 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetFirstAippInfo_res.md#id1 -->

## Feature Description

Obtains the configuration information of the model AIPP, including static AIPP and dynamic AIPP. When obtaining dynamic AIPP information of a model, only the following parameters in the [aclAippInfo](aclAippInfo.md) structure can be obtained: srcFormat, srcDatatype, srcDimNum, shapeCount, and outDims. The values of other parameters are invalid.

The calculation methods and order of the operations supported by AIPP are as follows: crop -\> color space conversion -\> mean subtraction/normalization -\>padding.

## Prototype

```c
aclError aclmdlGetFirstAippInfo(uint32_t modelId, size_t index, aclAippInfo *aippInfo)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | Model ID.<br>After the model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, the [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, the [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or the [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |
| index | Input | Index of the input in the model. |
| aippInfo | Output | Pointer to the AIPP configuration information obtained for the specified input. See [aclAippInfo](aclAippInfo.md) for the type definition.<br>For details and parameter descriptions, see "Advanced Features > Enabling AIPP" in *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
