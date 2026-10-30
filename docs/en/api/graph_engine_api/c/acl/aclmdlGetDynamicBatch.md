# aclmdlGetDynamicBatch

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:13:08.277Z pushedAt=2026-10-29T03:12:24.711Z nl=2 -->

## Product Support

<!-- npu="A3" id588 -->
- Atlas A3 products: Supported
<!-- end id588 -->
<!-- npu="910b" id589 -->
- Atlas A2 products: Supported
<!-- end id589 -->
<!-- npu="310b" id590 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id590 -->
<!-- npu="310p" id591 -->
- Atlas inference products: Supported
<!-- end id591 -->
<!-- npu="910" id592 -->
- Atlas training products: Supported
<!-- end id592 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDynamicBatch_res.md#id1 -->

## Feature Description

Obtains the dynamic batch information supported by the model based on the model description.

## Prototype

```c
aclError aclmdlGetDynamicBatch(const aclmdlDesc *modelDesc, aclmdlBatch *batch)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| batch | Output | When batchCount is 0, batch level configuration is not supported, and the batch levels in the model prevail. |

<br>

The definition of aclmdlBatch is as follows:

```c
const int ACL_MAX_BATCH_NUM = 128;
    size_t batchCount; /** Number of batch levels supported by the model. */
    uint64_t batch[ACL_MAX_BATCH_NUM]; /** Specific batch levels supported by the model. */
} aclmdlBatch;
```

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
