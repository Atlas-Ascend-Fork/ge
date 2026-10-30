# aclmdlCreateAndGetOpDesc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:00:13.534Z pushedAt=2026-10-29T02:59:44.104Z nl=2 -->

## Product Support

<!-- npu="950" id50 -->
- Ascend 950PR&950DT products: Supported
<!-- end id50 -->
<!-- npu="A3" id51 -->
- Atlas A3 products: Supported
<!-- end id51 -->
<!-- npu="910b" id52 -->
- Atlas A2 products: Supported
<!-- end id52 -->
<!-- npu="310b" id53 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id53 -->
<!-- npu="310p" id54 -->
- Atlas inference products: Supported
<!-- end id54 -->
<!-- npu="910" id55 -->
- Atlas training products: Supported
<!-- end id55 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlCreateAndGetOpDesc_res.md#id1 -->

## Feature Description

Obtains the description information of a specified operator, including the operator name, input tensor descriptions, and output tensor descriptions. If the specified operator is not found, an error is returned. This API does not support dynamic shape scenarios.

**Example use case**: When an AI Core error occurs during whole-network model inference, you can call this API to obtain the description information of the faulty operator for further troubleshooting.

**The recommended API call sequence is as follows:**

1. Define and implement the exception callback function fn. For the callback function prototype, see aclrtSetExceptionInfoCallback.

    The key steps for implementing the callback function are as follows:

    1. In the exception callback function fn, call aclrtGetDeviceIdFromExceptionInfo, aclrtGetStreamIdFromExceptionInfo, and aclrtGetTaskIdFromExceptionInfo to obtain the Device ID, Stream ID, and Task ID, respectively.
    2. In the exception callback function fn, call [aclmdlCreateAndGetOpDesc](aclmdlCreateAndGetOpDesc.md) to obtain the operator description information.
    3. In the exception callback function fn, call [aclGetTensorDescByIndex](aclGetTensorDescByIndex.md) to obtain the input/output tensor descriptions of the specified operator.
    4. In the exception callback function fn, call the following APIs to obtain the data in the tensor descriptions for further analysis.

        For example, call the [aclGetTensorDescAddress](aclGetTensorDescAddress.md) API to obtain the memory address of the tensor data (from which you can obtain the tensor data), call the [aclGetTensorDescType](aclGetTensorDescType.md) API to obtain the data type in the tensor description, call the [aclGetTensorDescFormat](aclGetTensorDescFormat.md) API to obtain the format in the tensor description, call the [aclGetTensorDescNumDims](aclGetTensorDescNumDims.md) API to obtain the number of shape dimensions in the tensor description, and call the [aclGetTensorDescDimV2](aclGetTensorDescDimV2.md) API to obtain the size of a specified dimension in the shape.

2. Call the aclrtSetExceptionInfoCallback API to set the exception callback function.
3. Perform model inference.

    If an AI Core error occurs, the callback function fn is triggered to obtain the operator information for further analysis.

## Prototype

```c
aclError aclmdlCreateAndGetOpDesc(uint32_t deviceId, uint32_t streamId, uint32_t taskId, char *opName, size_t opNameLen, aclTensorDesc **inputDesc, size_t *numInputs, aclTensorDesc **outputDesc, size_t *numOutputs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| deviceId | Input | Device ID. Call the aclrtGetDeviceIdFromExceptionInfo API to obtain the Device ID from the exception information, and use it as the input of this API. |
| streamId | Input | Stream ID. Call the aclrtGetStreamIdFromExceptionInfo API to obtain the Stream ID from the exception information, and use it as the input of this API. |
| taskId | Input | Task ID. Call the aclrtGetTaskIdFromExceptionInfo API to obtain the Task ID from the exception information, and use it as the input of this API. |
| opName | Output | Pointer to the operator name string. |
| opNameLen | Input | Length of the operator name string. If the length specified by the user is shorter than the actual operator name length, an error is returned. |
| inputDesc | Output | Pointer to the tensor descriptions of all operator inputs, pointing to the start address of a contiguous memory block. See [aclTensorDesc](aclTensorDesc.md) for the type definition. |
| numInputs | Output | Pointer to the number of inputs. |
| outputDesc | Output | Pointer to the tensor descriptions of all operator outputs, pointing to the start address of a contiguous memory block. See [aclTensorDesc](aclTensorDesc.md) for the type definition. |
| numOutputs | Output | Pointer to the number of outputs. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
