# aclmdlExecute

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:12:56.894Z pushedAt=2026-11-18T03:29:00.631Z nl=2 -->

## Product Support

<!-- npu="950" id755 -->
- Ascend 950PR&950DT products: Supported
<!-- end id755 -->
<!-- npu="A3" id756 -->
- Atlas A3 products: Supported
<!-- end id756 -->
<!-- npu="910b" id757 -->
- Atlas A2 products: Supported
<!-- end id757 -->
<!-- npu="310b" id758 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id758 -->
<!-- npu="310p" id759 -->
- Atlas inference products: Supported
<!-- end id759 -->
<!-- npu="910" id760 -->
- Atlas training products: Supported
<!-- end id760 -->
<!-- npu="IPV350" id761 -->
- IPV350: not supported
<!-- end id761 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlExecute_res.md#id1 -->

## Function Description

Executes inference on an om or om2 model until the inference result is returned.

Model loading, model execution, and model unloading must be performed in the same Context. For details about creating a Context, see aclrtCreateContext.

## Prototype

```c
aclError aclmdlExecute(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the inference model.<br>After a model loading API (for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) or [aclmdlLoadFromMem](aclmdlLoadFromMem.md)) is successfully called, a model ID is returned and used as the input of this API. |
| input | Input | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition. |
| output | Output | Pointer to the output data of model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition.<br>When aclCreateDataBuffer is called to create an aclDataBuffer that stores the output data of the corresponding index, nullptr can be passed to the data parameter and size must be set to 0, indicating that an empty aclDataBuffer is created. During model execution, the system internally calculates and allocates memory for the output of this index. This method saves memory, but after the memory data is used, the user must free the memory and reset the aclDataBuffer. In addition, memory copy is involved when the system internally allocates memory, which may cause performance loss. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- If business requirements make it necessary to use the same modelId in multiple threads, you must add locks between threads to ensure that the input/output memory is refreshed and that execution is a continuous operation. For example:

    ```c++
    // API call sequence of thread A:
    lock(handle1) -> aclrtMemcpy to refresh input/output memory -> aclmdlExecute to execute inference -> unlock(handle1)

    // API call sequence of thread B:
    lock(handle1) -> aclrtMemcpy to refresh input/output memory -> aclmdlExecute to execute inference -> unlock(handle1)
    ```

- The memory for storing model input/output data is Device memory. You can call APIs such as aclrtMalloc and hi\_mpi\_dvpp\_malloc to allocate Device memory. The hi\_mpi\_dvpp\_malloc API is a memory allocation API dedicated to media data processing. Generally, from a performance perspective, to reduce copies, the output of media data processing is used as the input of model inference to implement memory reuse. However, because the address space accessible to media data processing is limited, to ensure sufficient memory during media data processing, for functions other than media data processing (for example, model loading), you are advised to call other APIs, such as aclrtMalloc, to allocate memory.

## Example Code

The sample code for freeing memory and resetting aclDataBuffer is as follows:

```c
aclDataBuffer *dataBuffer = aclmdlGetDatasetBuffer(output, 0); // Obtain the corresponding dataBuffer by index.
void *data = aclGetDataBufferAddr(dataBuffer);  // Obtain the Device pointer of data.
aclrtFree(data ); // Free the Device memory.
aclUpdateDataBuffer(dataBuffer, nullptr, 0); // Reset the content in dataBuffer for the next inference.
```
