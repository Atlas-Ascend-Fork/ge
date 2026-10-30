# aclGetTensorDescDimV2

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:46:36.559Z pushedAt=2026-10-29T02:39:55.589Z nl=2 -->

## Product Support

<!-- npu="950" id818 -->
- Ascend 950PR&950DT products: Supported
<!-- end id818 -->
<!-- npu="A3" id819 -->
- Atlas A3 products: Supported
<!-- end id819 -->
<!-- npu="910b" id820 -->
- Atlas A2 products: Supported
<!-- end id820 -->
<!-- npu="310b" id821 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id821 -->
<!-- npu="310p" id822 -->
- Atlas inference products: Supported
<!-- end id822 -->
<!-- npu="910" id823 -->
- Atlas training products: Supported
<!-- end id823 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescDimV2_res.md#id1 -->

## Feature Description

Obtains the size of a specified dimension in a tensor description.

## Prototype

```c
aclError aclGetTensorDescDimV2(const aclTensorDesc *desc, size_t index, int64_t *dimSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type. |
| index | Input | Specifies the dimension whose size is to be obtained. The index value starts from 0.<br>Call the [aclGetTensorDescNumDims](aclGetTensorDescNumDims.md) API to obtain the number of shape dimensions. The value range of index is [0, (number of shape dimensions - 1)]. |
| dimSize | Output | Pointer to the size of the dimension specified by index in the tensor description.<br>If the returned dimSize value is -1, the dimension size is dynamic. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

When the return value of [aclGetTensorDescNumDims](aclGetTensorDescNumDims.md) is ACL\_UNKNOWN\_RANK, it indicates that the number of dimensions is unknown in a dynamic shape scenario, and the aclGetTensorDescDimV2 API cannot be called to obtain the size of a specified dimension.
