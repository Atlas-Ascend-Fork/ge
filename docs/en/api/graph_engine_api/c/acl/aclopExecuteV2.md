# aclopExecuteV2

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:48:43.392Z pushedAt=2026-10-29T03:44:02.490Z nl=2 -->

## Product Support

<!-- npu="950" id8 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id8 -->
<!-- npu="A3" id9 -->
- Atlas A3 products: Supported
<!-- end id9 -->
<!-- npu="910b" id10 -->
- Atlas A2 products: Supported
<!-- end id10 -->
<!-- npu="310b" id11 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id11 -->
<!-- npu="310p" id12 -->
- Atlas inference products: Supported
<!-- end id12 -->
<!-- npu="910" id13 -->
- Atlas training products: Supported
<!-- end id13 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopExecuteV2_res.md#id1 -->

## Feature Description

Executes the specified operator. This is an asynchronous API.

Each operator has different inputs, outputs, and attributes. The API locates the corresponding task based on information such as optype, the input tensor descriptions, the output tensor descriptions, and attr, and then dispatches it for execution. Therefore, when calling this API, you must organize the operator strictly according to its inputs, outputs, and attributes.

## Prototype

```c
aclError aclopExecuteV2(const char *opType,
int numInputs,
aclTensorDesc *inputDesc[],
aclDataBuffer *inputs[],
int numOutputs,
aclTensorDesc *outputDesc[],
aclDataBuffer *outputs[],
aclopAttr *attr,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of operator input tensors. |
| inputDesc | Input | Pointer array of operator input tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| inputs | Input | Pointer array of operator input tensors. See aclDataBuffer for the type definition.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the inputs array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| numOutputs | Input | Number of operator output tensors. |
| outputDesc | Input&Output | Pointer array of operator output tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| outputs | Output | Pointer array of operator output tensors. See aclDataBuffer for the type definition.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the outputs array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| attr | Input | Pointer to the operator attributes. See [aclopAttr](aclopAttr.md) for the type definition.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| stream | Input | Stream to which the operator is loaded. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- In multi-thread scenarios, specifying the same Stream or using the default Stream when calling this API is not supported; otherwise, task execution may be abnormal.

- For operators that support dynamic Shape, when the operator output Shape cannot be determined, call [aclopInferShape](aclopInferShape.md) to obtain the operator output Shape:
    - If the accurate output Shape can be obtained, use the accurate output Shape to construct the outputDesc parameter as the input of aclopExecuteV2. In this scenario, aclopExecuteV2 is an **asynchronous API**. For an asynchronous API, a successful call only indicates that the task has been dispatched, not that the task has been executed. After calling this API, call a synchronization wait API (for example, aclrtSynchronizeStream) to ensure that the task has been executed.
    - If the accurate output Shape cannot be obtained and only the output Shape range can be obtained, use the maximum Shape to construct the outputDesc parameter as the input of aclopExecuteV2. In this scenario, after calling aclopExecuteV2 to execute the operator, the system internally calculates the accurate output Shape and outputs it through the outputDesc parameter of aclopExecuteV2. At this time, aclopExecuteV2 is a **synchronous API**.
    - (This scenario is reserved.) If neither the accurate output Shape nor the Shape range can be obtained, you need to estimate a maximum Shape to construct the outputDesc parameter as the input of aclopExecuteV2. In this scenario, after calling aclopExecuteV2 to execute the operator, the system internally calculates the accurate output Shape and outputs it through the outputDesc parameter of aclopExecuteV2. At this time, aclopExecuteV2 is a **synchronous API**.

- When executing an operator with optional inputs, if an optional input is not used:
    - Call the aclCreateTensorDesc(ACL\_DT\_UNDEFINED, 0, nullptr, ACL\_FORMAT\_UNDEFINED) API to create data of the aclTensorDesc type, set the data type to ACL\_DT\_UNDEFINED, set Format to ACL\_FORMAT\_UNDEFINED, and set the Shape information to nullptr.
    - Call the aclCreateDataBuffer(nullptr, 0) API to create data of the aclDataBuffer type. The data in aclDataBuffer does not need to be released because it is a null pointer.

- When executing an operator with constant inputs, call [aclSetTensorConst](aclSetTensorConst.md) first to set the constant inputs.

    When calling the [aclopCompile](aclopCompile.md) API and the aclopExecuteV2 API, the constant input data that is set must be consistent.

    <!-- npu="950,A3,910b,910,310p,310b" id1 -->
    For scenarios where an operator has constant inputs, if the [aclSetTensorConst](aclSetTensorConst.md) API is not called to set the constant inputs, call the [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) API to set the placement attribute of the TensorDesc and set memType to Host memory.
    <!-- end id1 -->

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
- When executing a single operator, you are generally required to place the input/output tensor data in Device memory, for example, in a scenario where two tensors are added. However, for some operators, in addition to taking tensor data in Device memory, such as featureMap and weight, as inputs, they also take tensor data that is usually in Host memory, such as tensor shape information, learningRate, and tensor dims information, as inputs. At this time, you do not need to copy the tensor data in Host memory to the Device as inputs. You only need to call the [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) API to set the placement attribute of the corresponding TensorDesc to Host memory. When executing a single operator, for a TensorDesc whose placement attribute is set to Host memory, the corresponding tensor data must be stored in Host memory, and the API internally copies the tensor data from the Host to the Device automatically.
<!-- end id2 -->
