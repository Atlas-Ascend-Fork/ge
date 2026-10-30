# aclCreateTensorDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:35:40.017Z pushedAt=2026-10-29T02:32:49.308Z nl=2 -->

## Product Support

<!-- npu="950" id993 -->
- Ascend 950PR&950DT products: Supported
<!-- end id993 -->
<!-- npu="A3" id994 -->
- Atlas A3 products: Supported
<!-- end id994 -->
<!-- npu="910b" id995 -->
- Atlas A2 products: Supported
<!-- end id995 -->
<!-- npu="310b" id996 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id996 -->
<!-- npu="310p" id997 -->
- Atlas inference products: Supported
<!-- end id997 -->
<!-- npu="910" id998 -->
- Atlas training products: Supported
<!-- end id998 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclCreateTensorDesc_res.md#id1 -->

## Feature Description

Creates data of the aclTensorDesc type, which describes information such as the data type, shape, and format of a tensor.

To destroy data of the aclTensorDesc type, see [aclDestroyTensorDesc](aclDestroyTensorDesc.md).

## Prototype

```c
aclTensorDesc *aclCreateTensorDesc(aclDataType dataType,
int numDims,
const int64_t *dims,
aclFormat format)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| dataType | Input | Data type of the tensor description. See aclDataType for the type definition. |
| numDims | Input | Number of dimensions of the shape described by the tensor.<br>numDims is subject to the following constraints:<br><br>  - numDims must be greater than or equal to 0.<br>  - When numDims > 0, the value of numDims must be consistent with the length of the dims array.<br>  - When numDims = 0, the system does not use the dims array values, and the dims parameter value is invalid. |
| dims | Input | Pointer to the dimension sizes of the tensor description.<br>dims is an array in which each element indicates the size of each dimension of the tensor. If the value of an element in the array is 0, the tensor is an empty tensor.<br>If you need to use an empty tensor, the minimum memory size to be allocated is 1 byte to ensure normal subsequent service running. |
| format | Input | Format of the tensor description. See aclFormat for the type definition. |

## Return Value

Returns a pointer of the [aclTensorDesc](aclTensorDesc.md) type.
