# aclSetTensorValueRange

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:05:20.164Z pushedAt=2026-10-29T04:00:59.051Z nl=2 -->

## Product Support

<!-- npu="950" id1035 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1035 -->
<!-- npu="A3" id1036 -->
- Atlas A3 products: Supported
<!-- end id1036 -->
<!-- npu="910b" id1037 -->
- Atlas A2 products: Supported
<!-- end id1037 -->
<!-- npu="310b" id1038 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1038 -->
<!-- npu="310p" id1039 -->
- Atlas inference products: Supported
<!-- end id1039 -->
<!-- npu="910" id1040 -->
- Atlas training products: Supported
<!-- end id1040 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorValueRange_res.md#id1 -->

## Feature Description

After a tensor description is created by calling [aclCreateTensorDesc](aclCreateTensorDesc.md), you can use this API to set the value range of the tensor data.

Use scenario: For some operators, the input value is the output shape of the operator. In dynamic shape scenarios, this shape value has a value range. When executing the operator, you need to set the shape value range on the input (for example: [[16,16],[1,128],[20,20],[1,10]\]) so that the operator can be compiled and executed properly.

## Prototype

```c
aclError aclSetTensorValueRange(aclTensorDesc* desc, size_t valueCount, int64_t valueRange[][ACL_TENSOR_VALUE_RANGE_NUM])
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| valueCount | Input | Number of data values whose ranges need to be set. |
| valueRange | Input | valueRange specifies the range of each data value, represented by a two-dimensional array.<br>#define ACL_TENSOR_VALUE_RANGE_NUM 2 |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
