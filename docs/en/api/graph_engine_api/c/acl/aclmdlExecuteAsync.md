# aclmdlExecuteAsync

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-17T14:13:17.971Z pushedAt=2026-11-18T03:29:06.685Z nl=2 -->

## Product Support

<!-- npu="950" id295 -->
- Ascend 950PR&950DT products: Supported
<!-- end id295 -->
<!-- npu="A3" id296 -->
- Atlas A3 products: Supported
<!-- end id296 -->
<!-- npu="910b" id297 -->
- Atlas A2 products: Supported
<!-- end id297 -->
<!-- npu="310b" id298 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id298 -->
<!-- npu="310p" id299 -->
- Atlas inference products: Supported
<!-- end id299 -->
<!-- npu="910" id300 -->
- Atlas training products: Supported
<!-- end id300 -->
<!-- npu="IPV350" id301 -->
- IPV350: not supported
<!-- end id301 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlExecuteAsync_res.md#id1 -->

## Function Description

Executes om or om2 model inference. This is an asynchronous API.

## Prototype

```c
aclError aclmdlExecuteAsync(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output, aclrtStream stream)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelId | Input | ID of the model to be inferred.<br>After a model loading API (for example, [aclmdlLoadFromFile](aclmdlLoadFromFile.md) or [aclmdlLoadFromMem](aclmdlLoadFromMem.md)) is successfully called, a model ID is returned and used as the input of this API. |
| input | Input | Pointer to the input data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition. |
| output | Output | Pointer to the output data for model inference. See [aclmdlDataset](aclmdlDataset.md) for the type definition. |
| stream | Input | Stream to be specified. See aclrtStream for the type definition. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

- For a model with the same modelId, do not call aclmdlExecuteAsync to perform model inference in a multi-stream concurrent scenario. The following is an incorrect example, in which two aclmdlExecuteAsync calls are executed concurrently across multiple streams, causing an error:

    ```c++
    //......
    aclmdlExecuteAsync(modelId1, input, output, stream1);
    aclmdlExecuteAsync(modelId1, input, output, stream2);
    aclrtSynchronizeStream(stream1);
    aclrtSynchronizeStream(stream2);
    //......
    ```

- If business requirements make it necessary to use the same modelId in multiple threads, you must add locks between threads to ensure that the input/output memory is refreshed and that execution is a continuous operation. For example:

    ```c++
    // API call sequence of thread A:
    lock(handle1) -> aclrtMemcpyAsync(stream1) refresh input/output memory -> aclmdlExecuteAsync(modelId1,stream1) perform inference -> unlock(handle1)

    // API call sequence of thread B:
    lock(handle1) -> aclrtMemcpyAsync(stream1) refresh input/output memory -> aclmdlExecuteAsync(modelId1,stream1) perform inference -> unlock(handle1)
    ```

- If an external Allocator is required, the stream used when registering the Allocator must be the same as the stream used during model execution.
- The memory for storing model input/output data is Device memory. You can call APIs such as aclrtMalloc and hi\_mpi\_dvpp\_malloc to allocate Device memory. The hi\_mpi\_dvpp\_malloc API is a memory allocation API dedicated to media data processing. Generally, from a performance perspective, to reduce copies, the output of media data processing is used as the input of model inference to implement memory reuse. However, because the address space accessible to media data processing is limited, to ensure sufficient memory during media data processing, for functions other than media data processing (for example, model loading), you are advised to call other APIs, such as aclrtMalloc, to allocate memory.
