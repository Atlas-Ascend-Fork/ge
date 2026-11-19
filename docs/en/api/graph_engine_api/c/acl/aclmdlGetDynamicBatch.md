# aclmdlGetDynamicBatch

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:20:09.640Z pushedAt=2026-11-18T03:30:55.084Z nl=2 -->

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
<!-- npu="IPV350" id593 -->
- IPV350: not supported
<!-- end id593 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDynamicBatch_res.md#id1 -->

## Function Description

Obtains the dynamic batch information supported by the model based on the om/om2 model description.

## Prototype

```c
aclError aclmdlGetDynamicBatch(const aclmdlDesc *modelDesc, aclmdlBatch *batch)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| batch | Output | When batchCount is 0, batch level configuration is not supported, and the batch levels in the model prevail. |

<br>

The definition of aclmdlBatch is as follows:

```c
const int ACL_MAX_BATCH_NUM = 128;
    size_t batchCount; /** Number of batch batching levels supported in the model */
    uint64_t batch[ACL_MAX_BATCH_NUM]; /** Specific batching levels supported in the model */
} aclmdlBatch;
```

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
