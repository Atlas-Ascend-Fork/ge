# aclmdlExecute

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:06:33.701Z pushedAt=2026-10-29T03:06:01.206Z nl=2 -->

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlExecute_res.md#id1 -->

## Feature Description

Executes model inference until the inference result is returned.

Model loading, model execution, and model unloading must be performed in the same Context (see also aclrtCreateContext for Context creation).

## Prototype

```c
aclError aclmdlExecute(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the inference model.<br>After a model loading API (for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) or [aclmdlLoadFromMem](aclmdlLoadFromMem.md)) is successfully called, a model ID is returned and used as the input of this API. |
| input | Input | Pointer to the input data for model inference. For the type definition, see [aclmdlDataset](aclmdlDataset.md). |
| output | Output | Pointer to the output data of model inference. For the type definition, see [aclmdlDataset](aclmdlDataset.md).<br>When calling the aclCreateDataBuffer API to create an aclDataBuffer for storing the output data of the corresponding index, you can pass nullptr in the data parameter and set size to 0 to create an empty aclDataBuffer. During model execution, the system internally calculates and allocates memory for the output of this index. This approach saves memory, but after the memory data is used, you must release the memory and reset the aclDataBuffer. In addition, memory allocation by the system involves memory copy, which may cause performance loss. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- If business requirements dictate that the same modelId must be used in multiple threads, you must add locks between user threads to ensure that refreshing the input/output memory and execution are continuous operations. For example:

    ```c++
    // API call sequence of thread A:
    lock(handle1) -> aclrtMemcpy refreshes the input/output memory -> aclmdlExecute performs inference -> unlock(handle1)

    // API call sequence of thread B:
    lock(handle1) -> aclrtMemcpy refreshes the input/output memory -> aclmdlExecute performs inference -> unlock(handle1)
    ```

- The memory that stores model input/output data is Device memory. You can call aclrtMalloc, hi\_mpi\_dvpp\_malloc, and other APIs to allocate Device memory. The hi\_mpi\_dvpp\_malloc API is a memory allocation API dedicated to media data processing. Generally, from a performance perspective, to reduce copies, the output of media data processing is used as the input of model inference to implement memory reuse. However, because the address space accessible to media data processing is limited, to ensure sufficient memory during media data processing, for functions other than media data processing (for example, model loading), you are advised to call other APIs, such as aclrtMalloc, to allocate memory.

## Example Code

The following example code shows how to release memory and reset aclDataBuffer:

```c
aclDataBuffer *dataBuffer = aclmdlGetDatasetBuffer(output, 0); // Obtain the corresponding dataBuffer by index.
void *data = aclGetDataBufferAddr(dataBuffer);  // Obtain the Device pointer of data.
aclrtFree(data ); // Release the Device memory.
aclUpdateDataBuffer(dataBuffer, nullptr, 0); // Reset the content in dataBuffer for the next inference.
```
