# aclmdlGetDescFromMem

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:12:45.291Z pushedAt=2026-10-29T03:12:12.132Z nl=2 -->

## Product Support

<!-- npu="950" id407 -->
- Ascend 950PR&950DT products: Supported
<!-- end id407 -->
<!-- npu="A3" id408 -->
- Atlas A3 products: Supported
<!-- end id408 -->
<!-- npu="910b" id409 -->
- Atlas A2 products: Supported
<!-- end id409 -->
<!-- npu="310b" id410 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id410 -->
<!-- npu="310p" id411 -->
- Atlas inference products: Supported
<!-- end id411 -->
<!-- npu="910" id412 -->
- Atlas training products: Supported
<!-- end id412 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDescFromMem_res.md#id1 -->

## Feature Description

Obtains the model description of the model from memory.

The model description obtained through this API cannot be used with the [aclmdlGetOpAttr](aclmdlGetOpAttr.md) and [aclmdlGetCurOutputDims](aclmdlGetCurOutputDims.md) APIs.

## Prototype

```c
aclError aclmdlGetDescFromMem(aclmdlDesc *modelDesc, const void *model, size_t modelSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Output | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| model | Input | Memory address pointer that stores the model data. |
| modelSize | Input | Length of the model data in memory, in bytes. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP form, the memory pointed to by model must be allocated on the Host.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC form, the memory pointed to by model must be allocated on the Device.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open form, the memory pointed to by model must be allocated on the Device.
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDescFromMem_res.md#id2 -->
