# aclmdlGetOutputSizeByIndex

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:23:50.964Z pushedAt=2026-10-29T03:23:17.407Z nl=2 -->

## Product Support

<!-- npu="950" id141 -->
- Ascend 950PR&950DT products: Supported
<!-- end id141 -->
<!-- npu="A3" id142 -->
- Atlas A3 products: Supported
<!-- end id142 -->
<!-- npu="910b" id143 -->
- Atlas A2 products: Supported
<!-- end id143 -->
<!-- npu="310b" id144 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id144 -->
<!-- npu="310p" id145 -->
- Atlas inference products: Supported
<!-- end id145 -->
<!-- npu="910" id146 -->
- Atlas training products: Supported
<!-- end id146 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOutputSizeByIndex_res.md#id1 -->

## Feature Description

Obtains the size of a specified output based on the model description, in bytes.

## Prototype

```c
size_t aclmdlGetOutputSizeByIndex(aclmdlDesc *modelDesc, size_t index)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type. Call the [aclmdlCreateDesc](aclmdlCreateDesc.md) API in advance to create data of the aclmdlDesc type. |
| index | Input | Specifies the index of the output whose size is to be obtained. The index value starts from 0. |

## Return Value

For dynamic batch and dynamic resolution (width and height) scenarios, the size of the output corresponding to the maximum profile is returned. For static scenarios, the size of the specified output is returned. The unit is bytes.

## Constraints

If the size obtained through this API is 0, it may be because the output shape range is uncertain. The following two methods are supported:

- Method 1: **The system internally applies for the output memory of the corresponding index**, which saves memory. However, after the memory data is used, you need to free the memory. In addition, memory copy is involved when the system internally applies for memory, which may cause performance loss. This method is supported only when using the [aclmdlExecute](aclmdlExecute.md) and [aclmdlExecuteV2](aclmdlExecuteV2.md) inference APIs.

    When calling aclCreateDataBuffer to create an aclDataBuffer type for storing the **output data** of the corresponding index, **you can pass nullptr in the data parameter and set size to 0**, which means creating an empty aclDataBuffer type. Then, during model execution, the system **internally calculates and applies for** the output memory of this index. This method saves memory, but after the memory data is used, you need to free the memory and reset the aclDataBuffer. In addition, memory copy is involved when the system internally applies for memory, which may cause performance loss.

    The sample code for freeing memory and resetting aclDataBuffer is as follows:

    ```c++
    aclDataBuffer *dataBuffer = aclmdlGetDatasetBuffer(output, 0); // Obtain the corresponding dataBuffer by index.
    void *data = aclGetDataBufferAddr(dataBuffer);  // Obtain the Device pointer of data.
    aclrtFree(data ); // Free the Device memory.
    aclUpdateDataBuffer(dataBuffer, nullptr, 0); // Reset the content in dataBuffer for the next inference.
    ```

- Method 2: **You estimate the output memory size and apply for memory**, and manage the memory by yourself. However, the memory size may be insufficient or excessive. If it is insufficient, the system reports an error during verification; if it is excessive, memory is wasted.

    You need to first estimate a sufficiently large output memory size based on the actual situation. During model execution, the system verifies whether the output memory size you specified meets the requirements. If it does not, the system returns an error, and the error message indicates the exact output memory size required. You can view the error in either of the following two ways:

    - Obtain the application log file and view the ERROR-level error.
    - Call the aclGetRecentErrMsg API in the application to obtain the error.
