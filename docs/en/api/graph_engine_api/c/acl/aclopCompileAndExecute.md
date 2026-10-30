# aclopCompileAndExecute

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:43:30.154Z pushedAt=2026-10-29T03:42:36.724Z nl=2 -->

## Product Support

<!-- npu="950" id253 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id253 -->
<!-- npu="A3" id254 -->
- Atlas A3 products: Supported
<!-- end id254 -->
<!-- npu="910b" id255 -->
- Atlas A2 products: Supported
<!-- end id255 -->
<!-- npu="310b" id256 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id256 -->
<!-- npu="310p" id257 -->
- Atlas inference products: Supported
<!-- end id257 -->
<!-- npu="910" id258 -->
- Atlas training products: Supported
<!-- end id258 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCompileAndExecute_res.md#id1 -->

## Feature Description

Compiles and executes the specified operator. Currently, only operators with a fixed Shape are supported. This is an asynchronous API.

Each operator has different inputs, outputs, and attributes. The API locates the corresponding task based on information such as optype, the input tensor description, the output tensor description, and attr, and then dispatches it for execution. Therefore, when calling this API, you must organize the operator strictly according to its inputs, outputs, and attributes.

Before compiling and executing the operator, you can call [aclSetCompileopt](aclSetCompileopt.md) to set compilation options.

## Prototype

```c
aclError aclopCompileAndExecute(const char *opType,
int numInputs,
const aclTensorDesc *const inputDesc[],
const aclDataBuffer *const inputs[],
int numOutputs,
const aclTensorDesc *const outputDesc[],
aclDataBuffer *const outputs[],
const aclopAttr *attr,
aclopEngineType engineType,
aclopCompileType compileFlag,
const char *opPath,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of operator input tensors. |
| inputDesc | Input | Pointer array of operator input tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the value of numInputs, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| inputs | Input | Pointer array of operator input tensors. See aclDataBuffer for the type definition.<br>Call the aclCreateDataBuffer API in advance to create data of the aclDataBuffer type.<br>The number of elements in the inputs array must be consistent with the value of numInputs, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| numOutputs | Input | Number of operator output tensors. |
| outputDesc | Input | Pointer array of operator output tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the value of numOutputs, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| outputs | Input&Output | Pointer array of operator output tensors. See aclDataBuffer for the type definition.<br>Call the aclCreateDataBuffer API in advance to create data of the aclDataBuffer type.<br>The number of elements in the outputs array must be consistent with the value of numOutputs, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| attr | Input | Pointer to the operator attributes. See [aclopAttr](aclopAttr.md) for the type definition.<br>Call the [aclopCreateAttr](aclopCreateAttr.md) API in advance to create the aclopAttr type. |
| engineType | Input | Operator execution engine. See [aclopEngineType](aclopEngineType.md) for the type definition. |
| compileFlag | Input | Operator compilation flag. See [aclopCompileType](aclopCompileType.md) for the type definition. |
| opPath | Input | Pointer to the path of the operator implementation file (*.py), excluding the file name. Reserved parameter, currently only nullptr is supported. |
| stream | Input | Stream to which the operator is loaded. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- In multi-thread scenarios, do not specify the same Stream or use the default Stream when calling this API. Otherwise, task execution may be abnormal.
- Each operator has different inputs, outputs, and attributes. When calling aclopCompileAndExecute, you must organize the operator strictly according to its inputs, outputs, and attributes. The API locates the corresponding task based on the optype, input tensor descriptions, output tensor descriptions, attr, and other information, and then compiles and executes the operator.
- When compiling and executing an operator with optional inputs, if an optional input is not used:
    - Call the aclCreateTensorDesc(ACL\_DT\_UNDEFINED, 0, nullptr, ACL\_FORMAT\_UNDEFINED) API to create data of the aclTensorDesc type, set the data type to ACL\_DT\_UNDEFINED, set Format to ACL\_FORMAT\_UNDEFINED, and set the shape information to nullptr.
    - Call the aclCreateDataBuffer(nullptr, 0) API to create data of the aclDataBuffer type. The data in aclDataBuffer does not need to be released because it is a null pointer.

- When compiling and executing an operator with constant inputs, call the [aclSetTensorConst](aclSetTensorConst.md) API first to set the constant inputs.

    <!-- npu="950,A3,910b,910,310p,310b" id1 -->
    For an operator with constant inputs, if the [aclSetTensorConst](aclSetTensorConst.md) API is not called to set the constant inputs, call [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) to set the placement attribute of TensorDesc and set memType to Host memory.
    <!-- end id1 -->

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
- When executing a single operator, the input/output tensor data you pass in is generally required to be stored in Device memory, for example, in the scenario of adding two tensors. However, for some operators, in addition to tensor data in Device memory such as featureMap and weight, tensor data that is usually in Host memory, such as tensor shape information, learningRate, and tensor dims information, is also required as input. In this case, you do not need to copy the tensor data in Host memory to the Device as input. Instead, call the [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) API to set the placement attribute of the corresponding TensorDesc to Host memory. When executing a single operator, for a TensorDesc whose placement attribute is set to Host memory, the corresponding tensor data must be stored in Host memory, and the API automatically copies the tensor data from the Host to the Device.
<!-- end id2 -->
