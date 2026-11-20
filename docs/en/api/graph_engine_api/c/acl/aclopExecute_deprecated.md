# aclopExecute (Deprecated)

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:02:17.492Z pushedAt=2026-11-18T03:42:03.343Z nl=2 -->

**Note: This API will be deprecated in later versions. Use the [aclopExecuteV2](aclopExecuteV2.md) API instead.**

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
<!-- npu="IPV350" id161 -->
- IPV350: not supported
<!-- end id161 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopExecute_deprecated_res.md#id1 -->

## Function Description

Executes the specified operator. This is an asynchronous API.

Each operator has different inputs, outputs, and attributes. The API locates the corresponding task based on information such as optype, the input tensor descriptions, the output tensor descriptions, and attr, and then dispatches it for execution. Therefore, when calling this API, you must organize the operator strictly according to its inputs, outputs, and attributes.

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
| numInputs | Input | Number of input tensors of the operator. |
| inputDesc | Input | Pointer array of operator input tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| inputs | Input | Pointer array of operator input tensors. See aclDataBuffer for the type definition.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the inputs array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| numOutputs | Input | Number of operator output tensors.<br>The number of elements in the outputDesc array must be consistent with the numOutputs parameter value. |
| outputDesc | Input | Pointer array of operator output tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| outputs | Output | Pointer array of operator output tensors. See aclDataBuffer for the type definition.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the outputs array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| attr | Input | Pointer to the operator attributes. See [aclopAttr](aclopAttr.md) for the type definition.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| stream | Input | Stream on which the operator is loaded. See also aclrtStream for type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

In a multithreaded scenario, do not specify the same Stream or use the default Stream when calling this API. Otherwise, task execution may be abnormal.
