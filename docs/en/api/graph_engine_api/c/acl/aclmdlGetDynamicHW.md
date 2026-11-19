# aclmdlGetDynamicHW

<!-- md-trans-meta sourceCommit=fcba9ddddeed35cd2a7a0ade347fab638d62fcb2 translatedAt=2026-11-17T14:20:26.195Z pushedAt=2026-11-18T03:30:59.050Z nl=2 -->

## Product Support

<!-- npu="A3" id512 -->
- Atlas A3 products: Supported
<!-- end id512 -->
<!-- npu="910b" id513 -->
- Atlas A2 products: Supported
<!-- end id513 -->
<!-- npu="310b" id514 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id514 -->
<!-- npu="310p" id515 -->
- Atlas inference products: Supported
<!-- end id515 -->
<!-- npu="910" id516 -->
- Atlas training products: Supported
<!-- end id516 -->
<!-- npu="IPV350" id517 -->
- IPV350: not supported
<!-- end id517 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDynamicHW_res.md#id1 -->

## Function Description

Obtains the dynamic width/height information supported by the model based on the om/om2 model description information.

## Prototype

```c
aclError aclmdlGetDynamicHW(const aclmdlDesc *modelDesc, size_t index, aclmdlHW *hw)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Reserved parameter, which is not used currently. Set it to -1. |
| hw | Output | When hwCount is 0, tier information is not supported, and the tiers in the model prevail. |

<br>

The aclmdlHW type is defined as follows:

```c
const int ACL_MAX_HW_NUM = 128;
typedef struct aclmdlHW {
    uint64_t hw[ACL_MAX_HW_NUM][2]; /** Specific gear levels supported in the model. In each gear level, array subscript 0 represents the height and subscript 1 represents the width. */
} aclmdlHW;
```

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
