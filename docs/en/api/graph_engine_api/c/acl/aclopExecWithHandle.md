# aclopExecWithHandle

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:49:27.471Z pushedAt=2026-10-29T03:44:08.409Z nl=2 -->

## Product Support

<!-- npu="950" id190 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id190 -->
<!-- npu="A3" id191 -->
- Atlas A3 products: Supported
<!-- end id191 -->
<!-- npu="910b" id192 -->
- Atlas A2 products: Supported
<!-- end id192 -->
<!-- npu="310b" id193 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id193 -->
<!-- npu="310p" id194 -->
- Atlas inference products: Supported
<!-- end id194 -->
<!-- npu="910" id195 -->
- Atlas training products: Supported
<!-- end id195 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopExecWithHandle_res.md#id1 -->

## Feature Description

Invokes an operator by handle. This API does not support dynamic shape operators. For dynamic shape operators, use [aclopExecuteV2](aclopExecuteV2.md). This is an asynchronous API.

## Prototype

```c
aclError aclopExecWithHandle(aclopHandle *handle,
int numInputs,
const aclDataBuffer *const inputs[],
int numOutputs,
aclDataBuffer *const outputs[],
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| handle | Input | Pointer to the operator execution handle. See also [aclopHandle](aclopHandle.md) for type definition.<br>Call the [aclopCreateHandle](aclopCreateHandle.md) API in advance to create data of the aclopHandle type. |
| numInputs | Input | Number of input tensors of the operator. |
| inputs | Input | Pointer array of input tensors of the operator. See also aclDataBuffer for type definition.<br>Call the aclCreateDataBuffer API in advance to create data of the aclDataBuffer type.<br>The number of elements in the inputs array must be consistent with the value of numInputs. |
| numOutputs | Input | Number of output tensors of the operator. |
| outputs | Output | Pointer array of output tensors of the operator. See also aclDataBuffer for type definition.<br>Call the aclCreateDataBuffer API in advance to create data of the aclDataBuffer type.<br>The number of elements in the outputs array must be consistent with the value of numOutputs. |
| stream | Input | Stream on which the operator is loaded. See also aclrtStream for type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- In a multithreaded scenario, do not specify the same Stream or use the default Stream when calling this API. Otherwise, task execution may be abnormal.
- When executing an operator with optional inputs, if an optional input is not used, call aclCreateDataBuffer(nullptr, 0) to create data of the aclDataBuffer type. The data in aclDataBuffer does not need to be released because it is a null pointer.
