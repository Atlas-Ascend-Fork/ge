# aclmdlGetDynamicHW

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:13:48.906Z pushedAt=2026-10-29T03:12:30.725Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetDynamicHW_res.md#id1 -->

## Feature Description

Obtains the dynamic width and height information supported by the model based on the model description.

## Prototype

```c
aclError aclmdlGetDynamicHW(const aclmdlDesc *modelDesc, size_t index, aclmdlHW *hw)
```

## Parameters

| parameter name | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) in advance to create data of the aclmdlDesc type. |
| index | Input | Reserved parameter, which is not used currently. Set it to -1. |
| hw | Output | When hwCount is 0, tier information is not supported, and the tiers in the model prevail. |

<br>

The aclmdlHW type is defined as follows:

```c
const int ACL_MAX_HW_NUM = 128;
typedef struct aclmdlHW {
    uint64_t hw[ACL_MAX_HW_NUM][2]; /** Specific tiers supported by the model. In each tier, array subscript 0 represents the height, and subscript 1 represents the width. */
} aclmdlHW;
```

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
