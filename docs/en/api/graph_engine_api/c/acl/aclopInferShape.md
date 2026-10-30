# aclopInferShape

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:50:12.826Z pushedAt=2026-10-29T03:48:45.843Z nl=2 -->

## Product Support

<!-- npu="950" id470 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id470 -->
<!-- npu="A3" id471 -->
- Atlas A3 products: Supported
<!-- end id471 -->
<!-- npu="910b" id472 -->
- Atlas A2 products: Supported
<!-- end id472 -->
<!-- npu="310b" id473 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id473 -->
<!-- npu="310p" id474 -->
- Atlas inference products: Supported
<!-- end id474 -->
<!-- npu="910" id475 -->
- Atlas training products: Supported
<!-- end id475 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopInferShape_res.md#id1 -->

## Feature Description

Infers the output shape of an operator based on the operator input shape and input values.

The output shape inference covers three scenarios:

- If the accurate output shape can be obtained through shape inference, the accurate output shape is returned.
- If the accurate output shape cannot be obtained through shape inference but the range of the output shape can be obtained, the dimension value of the dynamic dimension in the operator output tensor description is set to -1 in the outputDesc output parameter. In this scenario, you can call the [aclGetTensorDescDimRange](aclGetTensorDescDimRange.md) API to obtain the range value of the specified dimension in the tensor description.
- (Reserved) If neither the accurate output shape nor the shape range can be obtained through shape inference, the dimension value of the dynamic dimension in the operator output tensor description is set to -2 in the outputDesc output parameter.

## Prototype

```c
aclError aclopInferShape(const char *opType,
int numInputs,
aclTensorDesc *inputDesc[],
aclDataBuffer *inputs[],
int numOutputs,
aclTensorDesc *outputDesc[],
aclopAttr *attr)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type name. |
| numInputs | Input | Number of operator input tensors. |
| inputDesc | Input | Pointer array of operator input tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type.<br>The number of elements in the inputDesc array must be consistent with the numInputs parameter value. |
| inputs | Input | Pointer array of operator input tensors. See aclDataBuffer for the type definition.<br>Call the aclCreateDataBuffer API in advance to create data of the aclDataBuffer type. |
| numOutputs | Input | Number of operator output tensors. |
| outputDesc | Output | Pointer array of operator output tensor descriptions. See [aclTensorDesc](aclTensorDesc.md) for the type definition.<br>Call the [aclCreateTensorDesc](aclCreateTensorDesc.md) API in advance to create the aclTensorDesc type.<br>The number of elements in the outputDesc array must be consistent with the numOutputs parameter value. |
| attr | Input | Operator attributes. See [aclopAttr](aclopAttr.md) for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In Ascend EP mode, the memory for the operator input tensor data at the inputs parameter must be allocated on the host.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In Ascend RC mode, the memory for the operator input tensor data at the inputs parameter must be allocated on the device.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In Control CPU open mode, the memory for the operator input tensor data at the inputs parameter must be allocated on the device.
<!-- end id3 -->

If the operator has dynamic inputs DYNAMIC\_INPUT or dynamic outputs DYNAMIC\_OUTPUT, before calling the aclopInferShape API to infer the output shapes of the operator, call the [aclSetTensorDescName](aclSetTensorDescName.md) API to set the names of the tensor descriptors of all inputs and outputs. The names must meet the following requirements:

- For required inputs, optional inputs, and required outputs, the names must be consistent with the input/output names defined in the operator IR prototype.
- For dynamic inputs and dynamic outputs, the names must be: the input/output name defined in the operator IR prototype + _number_. The number is determined by the count of dynamic inputs/outputs, starting from 0, where 0 corresponds to the first dynamic input/output, 1 corresponds to the second dynamic input/output, and so on.

For example, if an operator has two inputs (the first is the required input x, and the second is the dynamic input y with two inputs) and one required output z, the code example for setting the names by calling the [aclSetTensorDescName](aclSetTensorDescName.md) API is as follows:

```c++
aclSetTensorDescName(inputTensorDesc[0], "x");
aclSetTensorDescName(inputTensorDesc[1], "y0");
aclSetTensorDescName(inputTensorDesc[2], "y1");
aclSetTensorDescName(outputTensorDesc[0], "z");
```
