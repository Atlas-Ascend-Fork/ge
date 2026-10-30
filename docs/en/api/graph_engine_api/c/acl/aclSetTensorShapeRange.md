# aclSetTensorShapeRange

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:03:59.643Z pushedAt=2026-10-29T04:00:01.936Z nl=2 -->

## Product Support

<!-- npu="950" id386 -->
- Ascend 950PR&950DT products: Supported
<!-- end id386 -->
<!-- npu="A3" id387 -->
- Atlas A3 products: Supported
<!-- end id387 -->
<!-- npu="910b" id388 -->
- Atlas A2 products: Supported
<!-- end id388 -->
<!-- npu="310b" id389 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id389 -->
<!-- npu="310p" id390 -->
- Atlas inference products: Supported
<!-- end id390 -->
<!-- npu="910" id391 -->
- Atlas training products: Supported
<!-- end id391 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorShapeRange_res.md#id1 -->

## Feature Description

After you create the tensor description by calling [aclCreateTensorDesc](aclCreateTensorDesc.md), you can use this API to set the value range of each dimension of the tensor.

Use case: For operators with dynamic shapes, the variable dimensions in the input shape are represented by -1, but the range of each variable dimension differs and must be set explicitly. For example, if the shape is [16,-1,20,-1], the corresponding shape range can be [[16,16],[1,128],[20,20],[1,10]\], which means that the shape range of the first dimension is fixed at 16, the shape range of the second dimension is 1 to 128, the shape range of the third dimension is fixed at 20, and the shape range of the fourth dimension is 1 to 10.

## Prototype

```c
aclError aclSetTensorShapeRange(aclTensorDesc* desc, size_t dimsCount, int64_t dimsRange[][ACL_TENSOR_SHAPE_RANGE_NUM])
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create the aclTensorDesc type first. |
| dimsCount | Input | Number of dims in the tensor. |
| dimsRange | Input | Value range of each dimension, represented by a two-dimensional array.<br>If a dimension value in Shape is not -1, the dimension is fixed, and the maximum and minimum values in the corresponding shape range are the same. Otherwise, the dimension is dynamic, and the two elements of the array represent the minimum and maximum values of the corresponding tensor dims.<br>#define ACL_TENSOR_SHAPE_RANGE_NUM 2 |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
