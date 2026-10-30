# aclGetTensorDescByIndex

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T14:45:09.595Z pushedAt=2026-10-29T02:39:01.454Z nl=2 -->

## Product Support

<!-- npu="950" id1203 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1203 -->
<!-- npu="A3" id1204 -->
- Atlas A3 products: Supported
<!-- end id1204 -->
<!-- npu="910b" id1205 -->
- Atlas A2 products: Supported
<!-- end id1205 -->
<!-- npu="310b" id1206 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1206 -->
<!-- npu="310p" id1207 -->
- Atlas inference products: Supported
<!-- end id1207 -->
<!-- npu="910" id1208 -->
- Atlas training products: Supported
<!-- end id1208 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclGetTensorDescByIndex_res.md#id1 -->

## Feature Description

Obtains the tensor description of the operator at the specified index.

## Prototype

```c
aclTensorDesc *aclGetTensorDescByIndex(aclTensorDesc *desc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Input | Pointer to the aclTensorDesc type.<br>Call the [aclmdlCreateAndGetOpDesc](aclmdlCreateAndGetOpDesc.md) API to obtain the tensor descriptions of all operator inputs/outputs, which serve as the input of this API. |
| index | Input | Specifies which input/output tensor description to obtain. The index value starts from 0.<br>After you call the [aclmdlCreateAndGetOpDesc](aclmdlCreateAndGetOpDesc.md) API to obtain the number of operator inputs/outputs, the value range of this index is: [0, (number of operator inputs/outputs - 1)]. |

## Return Value

Returns a pointer to the [aclTensorDesc](aclTensorDesc.md) data of the specified input/output.
