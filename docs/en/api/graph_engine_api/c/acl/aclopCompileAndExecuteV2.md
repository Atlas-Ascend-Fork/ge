# aclopCompileAndExecuteV2

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T14:57:40.497Z pushedAt=2026-11-18T03:40:48.697Z nl=2 -->

## Product Support

<!-- npu="950" id309 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id309 -->
<!-- npu="A3" id310 -->
- Atlas A3 products: Supported
<!-- end id310 -->
<!-- npu="910b" id311 -->
- Atlas A2 products: Supported
<!-- end id311 -->
<!-- npu="310b" id312 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id312 -->
<!-- npu="310p" id313 -->
- Atlas inference products: Supported
<!-- end id313 -->
<!-- npu="910" id314 -->
- Atlas training products: Supported
<!-- end id314 -->
<!-- npu="IPV350" id315 -->
- IPV350: not supported
<!-- end id315 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCompileAndExecuteV2_res.md#id1 -->

## Function Description

Compiles and executes the specified operator. Currently, only operators with a fixed shape are supported. Each operator has different inputs, outputs, and attributes. The API locates the corresponding task based on information such as optype, input tensor descriptions, output tensor descriptions, and attr, and then dispatches it for execution. Therefore, when calling this API, you must organize the operator strictly according to its inputs, outputs, and attributes.

Before compiling and executing the operator, you can call [aclSetCompileopt](aclSetCompileopt.md) to set compilation options.

## Prototype

```c
aclError aclopCompileAndExecuteV2(const char *opType,
int numInputs,
aclTensorDesc *inputDesc[],
aclDataBuffer *inputs[],
int numOutputs,
aclTensorDesc *outputDesc[],
aclDataBuffer *outputs[],
aclopAttr *attr,
aclopEngineType engineType,
aclopCompileType compileFlag,
const char *opPath,
aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of input tensors of the operator. |
| inputDesc | Input | Pointer array of operator input tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| inputs | Input | Pointer array of operator input tensors. See aclDataBuffer for the type definition.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the inputs array must be consistent with the numInputs parameter value, and the elements in the inputs array and the inputDesc array must be in one-to-one correspondence. |
| numOutputs | Input | Number of output tensors of the operator. |
| outputDesc | Input&Output | Pointer array of operator output tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| outputs | Input&Output | Pointer array of operator output tensors. See aclDataBuffer for the type definition.<br>Call aclCreateDataBuffer in advance to create data of the aclDataBuffer type.<br>The number of elements in the outputs array must be consistent with the numOutputs parameter value, and the elements in the outputs array and the outputDesc array must be in one-to-one correspondence. |
| attr | Input | Pointer to the operator attributes. See [aclopAttr](aclopAttr.md) for the type definition.<br>Call [aclopCreateAttr](aclopCreateAttr.md) in advance to create the aclopAttr type. |
| engineType | Input | Operator execution engine. See [aclopEngineType](aclopEngineType.md) for the type definition. |
| compileFlag | Input | Operator compilation flag. See [aclopCompileType](aclopCompileType.md) for the type definition. |
| opPath | Input | Pointer to the path of the operator implementation file (*.py), excluding the file name. This is a reserved parameter and currently supports only nullptr. |
| stream | Input | Stream on which the operator is loaded. See also aclrtStream for type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- In a multithreaded scenario, do not specify the same Stream or use the default Stream when calling this API. Otherwise, task execution may be abnormal.
- When compiling and executing an operator with optional inputs, if an optional input is not used:
    - Call the aclCreateTensorDesc(ACL\_DT\_UNDEFINED, 0, nullptr, ACL\_FORMAT\_UNDEFINED) API to create data of the aclTensorDesc type, set the data type to ACL\_DT\_UNDEFINED, set the format to ACL\_FORMAT\_UNDEFINED, and set the shape information to nullptr.
    - Call the aclCreateDataBuffer(nullptr, 0) API to create data of the aclDataBuffer type. The data in aclDataBuffer does not need to be released because it is a null pointer.

- When compiling and executing an operator with constant inputs, call the [aclSetTensorConst](aclSetTensorConst.md) API first to set the constant inputs.

    <!-- npu="950,A3,910b,910,310p,310b" id1 -->
    For scenarios where an operator has constant inputs, if the [aclSetTensorConst](aclSetTensorConst.md) API is not called to set the constant inputs, call the [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) API to set the placement attribute of the TensorDesc and set memType to Host memory.
    <!-- end id1 -->

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
- When executing a single operator, you are generally required to place the input/output tensor data in Device memory, for example, in a scenario where two tensors are added. However, for some operators, in addition to taking tensor data in Device memory, such as featureMap and weight, as inputs, they also take tensor data that is usually in Host memory, such as tensor shape information, learningRate, and tensor dims information, as inputs. At this time, you do not need to copy the tensor data in Host memory to the Device as inputs. You only need to call the [aclSetTensorPlaceMent](aclSetTensorPlaceMent.md) API to set the placement attribute of the corresponding TensorDesc to Host memory. When executing a single operator, for a TensorDesc whose placement attribute is set to Host memory, the corresponding tensor data must be stored in Host memory, and the API internally copies the tensor data from the Host to the Device automatically.
<!-- end id2 -->
- For operators with dynamic shapes, when the output shape of the operator cannot be determined, call the [aclopInferShape](aclopInferShape.md) API to obtain the output shape of the operator:
    - If the accurate output shape can be obtained, use the accurate output shape to construct the outputDesc parameter as the input of aclopCompileAndExecuteV2. In this scenario, the aclopCompileAndExecuteV2 API is an **asynchronous API**. For an asynchronous API, a successful call only indicates that the task has been submitted successfully, not that the task has been executed successfully. After calling this API, call a synchronization wait API (for example, aclrtSynchronizeStream) to ensure that the task has been executed.
    - If the exact output Shape cannot be obtained and only the output Shape range is available, use the maximum Shape value to construct the outputDesc parameter as the input to aclopCompileAndExecuteV2. In this scenario, after the aclopCompileAndExecuteV2 API is called to execute the operator, the system internally computes the exact output Shape and outputs it through the outputDesc parameter of the aclopCompileAndExecuteV2 API. In this case, the aclopCompileAndExecuteV2 API is a **synchronous API**.
    - (This scenario is reserved.) If neither the exact output Shape nor the Shape range can be obtained, you need to estimate a maximum Shape to construct the outputDesc parameter as the input to aclopCompileAndExecuteV2. In this scenario, after the aclopCompileAndExecuteV2 API is called to execute the operator, the system internally computes the exact output Shape and outputs it through the outputDesc parameter of the aclopCompileAndExecuteV2 API. In this case, the aclopCompileAndExecuteV2 API is a **synchronous API**.
