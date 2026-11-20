# aclmdlGetAippType

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:15:56.139Z pushedAt=2026-11-18T03:29:40.102Z nl=2 -->

## Product Support

<!-- npu="950" id1161 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1161 -->
<!-- npu="A3" id1162 -->
- Atlas A3 products: Supported
<!-- end id1162 -->
<!-- npu="910b" id1163 -->
- Atlas A2 products: Supported
<!-- end id1163 -->
<!-- npu="310b" id1164 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1164 -->
<!-- npu="310p" id1165 -->
- Atlas inference products: Supported
<!-- end id1165 -->
<!-- npu="910" id1166 -->
- Atlas training products: Supported
<!-- end id1166 -->
<!-- npu="IPV350" id1167 -->
- IPV350: not supported
<!-- end id1167 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetAippType_res.md#id1 -->

## Function Description

Obtains the AIPP type (dynamic AIPP or static AIPP) supported by the specified input of a specified om or om2 model, and the index value corresponding to the dynamic AIPP input.

## Prototype

```c
aclError aclmdlGetAippType(uint32_t modelId, size_t index, aclmdlInputAippType *type, size_t *dynamicAttachedDataIndex)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the specified model.<br>After the model is loaded successfully by calling the [aclmdlLoadFromFile](aclmdlLoadFromFile.md) API, the [aclmdlLoadFromMem](aclmdlLoadFromMem.md) API, the [aclmdlLoadFromFileWithMem](aclmdlLoadFromFileWithMem.md) API, or the [aclmdlLoadFromMemWithMem](aclmdlLoadFromMemWithMem.md) API, the model ID is returned. |
| index | Input | Index of the input in the model. |
| type | Output | Pointer to the AIPP type of the specified model input. See [aclmdlInputAippType](aclmdlInputAippType.md) for the type definition. |
| dynamicAttachedDataIndex | Output | When type is not ACL_DATA_WITH_DYNAMIC_AIPP, this value is 0xFFFFFFFF, indicating that it is invalid.<br>When type is ACL_DATA_WITH_DYNAMIC_AIPP, this value is the index of the dynamic AIPP input (used to configure dynamic AIPP parameters). |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
