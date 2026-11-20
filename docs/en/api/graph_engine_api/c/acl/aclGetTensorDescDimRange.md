# aclGetTensorDescDimRange

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T13:48:24.953Z pushedAt=2026-11-18T03:10:51.550Z nl=2 -->

## Product Support

<!-- npu="950" id692 -->
- Ascend 950PR&950DT products: Supported
<!-- end id692 -->
<!-- npu="A3" id693 -->
- Atlas A3 products: Supported
<!-- end id693 -->
<!-- npu="910b" id694 -->
- Atlas A2 products: Supported
<!-- end id694 -->
<!-- npu="310b" id695 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id695 -->
<!-- npu="310p" id696 -->
- Atlas inference products: Supported
<!-- end id696 -->
<!-- npu="910" id697 -->
- Atlas training products: Supported
<!-- end id697 -->
<!-- npu="IPV350" id698 -->
- IPV350: not supported
<!-- end id698 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescDimRange_res.md#id1 -->

## Function Description

Obtains the range of a specified dimension in a tensor descriptor, where \[1,-1\] indicates the full Shape range.

When the return value of [aclGetTensorDescNumDims](aclGetTensorDescNumDims.md) is ACL\_UNKNOWN\_RANK, it indicates that the number of dimensions is unknown in the dynamic Shape scenario, and the aclGetTensorDescDimRange API cannot be called to obtain the range of a specified dimension.

## Prototype

```c
aclError aclGetTensorDescDimRange(const aclTensorDesc *desc, size_t index, size_t dimRangeNum, int64_t *dimRange)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| index | Input | Specifies the dimension whose size is to be obtained. The index value starts from 0.<br>Call the [aclGetTensorDescNumDims](aclGetTensorDescNumDims.md) API to obtain the number of shape dimensions. The value range of index is [0, (number of shape dimensions - 1)]. |
| dimRangeNum | Input | Length of dimRange. The value must be greater than or equal to 2. |
| dimRange | Output | Shape range of the dimension specified by index in the tensor descriptor.<br>dimRange is an array. The first element indicates the minimum value of the Shape range, and the second element indicates the maximum value. Only the first two elements in this array are valid.<br>When the value of the dimRange array is [1,-1], it indicates the full Shape range. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
