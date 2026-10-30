# aclopExecute

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:48:48.877Z pushedAt=2026-10-29T03:43:51.129Z nl=2 -->

## Product Support

<!-- npu="950" id155 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id155 -->
<!-- npu="A3" id156 -->
- Atlas A3 products: Supported
<!-- end id156 -->
<!-- npu="910b" id157 -->
- Atlas A2 products: Supported
<!-- end id157 -->
<!-- npu="310b" id158 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id158 -->
<!-- npu="310p" id159 -->
- Atlas inference products: Supported
<!-- end id159 -->
<!-- npu="910" id160 -->
- Atlas training products: Supported
<!-- end id160 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopExecute_deprecated_res.md#id1 -->

## Feature Description

Executes the specified operator. This is an asynchronous API.

Each operator has different inputs, outputs, and attributes. The API locates the corresponding task based on information such as optype, the input tensor description, the output tensor description, and attr, and then dispatches it for execution. Therefore, when calling this API, you must organize the operator strictly according to its inputs, outputs, and attributes.

## Prototype

```c
aclError aclopExecute(const char *opType,
int numInputs,
const aclTensorDesc *const inputDesc[],
const aclDataBuffer *const inputs[],
int numOutputs,
const aclTensorDesc *const outputDesc[],
aclDataBuffer *const outputs[],
const aclopAttr *attr,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of operator input tensors. |
| inputDesc | Input | Pointer array to the operator input tensor descriptions. For the type definition, see [aclTensorDesc](aclTensorDesc.md).<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must have a one-to-one correspondence. |
| inputs | Input | Pointer array to the operator input tensors. For the type definition, see aclDataBuffer.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the inputs array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must have a one-to-one correspondence. |
| numOutputs | Input | Number of operator output tensors.<br>The number of elements in the outputDesc array must be consistent with the numOutputs parameter value. |
| outputDesc | Input | Pointer array to the operator output tensor descriptions. For the type definition, see [aclTensorDesc](aclTensorDesc.md).<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must have a one-to-one correspondence. |
| outputs | Output | Pointer array to the operator output tensors. For the type definition, see aclDataBuffer.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the outputs array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must have a one-to-one correspondence. |
| attr | Input | Pointer to the operator attributes. For the type definition, see [aclopAttr](aclopAttr.md).<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| stream | Input | Stream to which the operator is loaded. For the type definition, see aclrtStream. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

In a multithreaded scenario, do not specify the same stream or use the default stream when calling this API; otherwise, task execution may be abnormal.
