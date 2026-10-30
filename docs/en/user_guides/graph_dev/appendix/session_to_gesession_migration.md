# Session to GeSession Migration Guide

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-19T12:36:41.505Z pushedAt=2026-10-21T07:12:49.488Z nl=2 -->

This section guides you through migrating from the Session class to the newly introduced GeSession class. GeSession is a refactored and optimized version of the original Session class. The main changes are as follows:

- Simplified compilation and loading process: You no longer need to manually call **CompileGraph** and **LoadGraph** (unless explicit control is required). For details, see [Compilation and Runtime API Changes](#compilation-and-runtime-api-changes).
- Unified parameter types for execution APIs: The inputs/outputs of all runtime  APIs are changed from **ge::Tensor** to **gert::Tensor**. For details, see [Tensor Type Changes](#tensor-type-changes).
- Optimized API naming and parameter types. For details, see [API Changes](#api-changes).

Notes on other changes:

- Header and library file changes: The header file is changed from **ge/ge\_api.h** to **ge/ge\_api\_v2.h**, and the library file is changed from **libge\_runner.so** to **libge\_runner\_v2.so**. For details, see [Library Link Changes](#library-link-changes).
- Asynchronous lifecycle: When using RunGraphAsync, inputs must remain valid until the callback is invoked. For details, see [Tensor Lifecycle](#li1).
- Mutually exclusive execution modes: The three modes cannot be mixed. For details, see [Compilation and Runtime API Changes](#compilation-and-runtime-api-changes).

The following sections describe the preceding changes in detail by category.

## Library Link Changes

**Table 1** Library link changes

|Category|Session|GeSession|
|--|--|--|
|Library file|libge_runner.so|libge_runner_v2.so|
|Header file|ge/ge_api.h|ge/ge_api_v2.h|

## API Changes

| Session API | GeSession API | Migration Notes |
| --- | --- | --- |
| [GEInitialize](../../../api/graph_engine_api/cpp/ge/Session/GEInitialize.md)(options) | [GEInitializeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEInitializeV2.md)(options) | New API |
| [GEFinalize](../../../api/graph_engine_api/cpp/ge/Session/GEFinalize.md)() | [GEFinalizeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEFinalizeV2.md)() | New API |
| [GEGetErrorMsg](../../../api/graph_engine_api/cpp/ge/Session/GEGetErrorMsg.md)() | [GEGetErrorMsgV3](../../../api/graph_engine_api/cpp/ge/GeSession/GEGetErrorMsgV3.md)() | New API |
| [GEGetWarningMsg](../../../api/graph_engine_api/cpp/ge/Session/GEGetWarningMsg.md)() | [GEGetWarningMsgV3](../../../api/graph_engine_api/cpp/ge/GeSession/GEGetWarningMsgV3.md)() | New API |
| [Session](../../../api/graph_engine_api/cpp/ge/Session/Session.md)(options) | [GeSession](../../../api/graph_engine_api/cpp/ge/GeSession/GESession.md)(options) | The constructors are basically the same, but only the ABI-compatible **std::map<AscendString, AscendString>** version is provided. |
| ~Session() | ~GeSession() | Destructor, no change |
| [AddGraph](../../../api/graph_engine_api/cpp/ge/Session/AddGraph.md)(uint32_t, const Graph&) | [AddGraph](../../../api/graph_engine_api/cpp/ge/GeSession/AddGraph.md)<br>(uint32_t, const Graph&) | The API remains the same. |
| [AddGraph](../../../api/graph_engine_api/cpp/ge/Session/AddGraph.md)(uint32_t, const Graph&, options) | [AddGraph](../../../api/graph_engine_api/cpp/ge/GeSession/AddGraph.md)<br>(uint32_t, const Graph&, options) | Only the ABI-compatible **std::map<AscendString, AscendString>** version is provided. |
| [AddGraphWithCopy](../../../api/graph_engine_api/cpp/ge/Session/AddGraphWithCopy.md) | [AddGraphClone](../../../api/graph_engine_api/cpp/ge/GeSession/AddGraphClone.md) | Renamed, with the same functionality |
| [RemoveGraph](../../../api/graph_engine_api/cpp/ge/Session/RemoveGraph.md) | [RemoveGraph](../../../api/graph_engine_api/cpp/ge/GeSession/RemoveGraph.md) | No change |
| [BuildGraph](../../../api/graph_engine_api/cpp/ge/Session/BuildGraph.md) | [CompileGraph](../../../api/graph_engine_api/cpp/ge/GeSession/CompileGraph.md) | Renamed, with the same functionality. **CompileGraph** of **GeSession** supports **Variable**. |
| [CompileGraph](../../../api/graph_engine_api/cpp/ge/Session/CompileGraph.md) | [CompileGraph](../../../api/graph_engine_api/cpp/ge/GeSession/CompileGraph.md) | Combines the functionality of **BuildGraph** and **CompileGraph**. |
| [LoadGraph](../../../api/graph_engine_api/cpp/ge/Session/LoadGraph.md) | [LoadGraph](../../../api/graph_engine_api/cpp/ge/GeSession/LoadGraph.md) | The API is basically the same, but the **GeSession** API automatically checks whether **CompileGraph** needs to be called first. |
| [RunGraph](../../../api/graph_engine_api/cpp/ge/Session/RunGraph.md) | [RunGraph](../../../api/graph_engine_api/cpp/ge/GeSession/RunGraph.md) | Important change: input/output changes from **ge::Tensor** to **gert::Tensor**. |
| [RunGraphWithStreamAsync](../../../api/graph_engine_api/cpp/ge/Session/RunGraphWithStreamAsync.md) | [RunGraphWithStreamAsync](../../../api/graph_engine_api/cpp/ge/GeSession/RunGraphWithStreamAsync.md) | Important change: input/output changes from **ge::Tensor** to **gert::Tensor**; **CompileGraph** and **LoadGraph** can be omitted. |
| [ExecuteGraphWithStreamAsync](../../../api/graph_engine_api/cpp/ge/Session/ExecuteGraphWithStreamAsync.md) | [RunGraphWithStreamAsync](../../../api/graph_engine_api/cpp/ge/GeSession/RunGraphWithStreamAsync.md) | **ExecuteGraphWithStreamAsync** (using **gert::Tensor**) of the **Session** class is merged into **RunGraphWithStreamAsync** of **GeSession**. |
| [RunGraphAsync](../../../api/graph_engine_api/cpp/ge/Session/RunGraphAsync.md) | [RunGraphAsync](../../../api/graph_engine_api/cpp/ge/GeSession/RunGraphAsync.md) | Important change: input/output changes from **ge::Tensor** to **gert::Tensor**; the callback signature changes from **RunAsyncCallback** to **RunAsyncCallbackV2**. |
| [RegisterCallBackFunc](../../../api/graph_engine_api/cpp/ge/Session/RegisterCallBackFunc.md) | [RegisterCallBackFunc](../../../api/graph_engine_api/cpp/ge/GeSession/RegisterCallBackFunc.md) | The callback signature changes, using the **RunCallback** type. |
| [GetCompiledGraphSummary](../../../api/graph_engine_api/cpp/ge/Session/GetCompiledGraphSummary.md) | [GetCompiledGraphSummary](../../../api/graph_engine_api/cpp/ge/GeSession/GetCompiledGraphSummary.md) | No change |
| [SetGraphConstMemoryBase](../../../api/graph_engine_api/cpp/ge/Session/SetGraphConstMemoryBase.md) | [SetGraphConstMemoryBase](../../../api/graph_engine_api/cpp/ge/GeSession/SetGraphConstMemoryBase.md) | No change |
| [UpdateGraphFeatureMemoryBase](../../../api/graph_engine_api/cpp/ge/Session/UpdateGraphFeatureMemoryBase.md) | [UpdateGraphFeatureMemoryBase](../../../api/graph_engine_api/cpp/ge/GeSession/UpdateGraphFeatureMemoryBase.md) | No change |
| [SetGraphFixedFeatureMemoryBase](../../../api/graph_engine_api/cpp/ge/Session/SetGraphFixedFeatureMemoryBase.md) | [SetGraphFixedFeatureMemoryBaseWithType](../../../api/graph_engine_api/cpp/ge/GeSession/SetGraphFixedFeatureMemoryBaseWithType.md) | The API name changes, and the **type** parameter is added. |
| [UpdateGraphRefreshableFeatureMemoryBase](../../../api/graph_engine_api/cpp/ge/Session/UpdateGraphRefreshableFeatureMemoryBase.md) | [UpdateGraphRefreshableFeatureMemoryBase](../../../api/graph_engine_api/cpp/ge/GeSession/UpdateGraphRefreshableFeatureMemoryBase.md) | No change |
| [RegisterExternalAllocator](../../../api/graph_engine_api/cpp/ge/Session/RegisterExternalAllocator.md) | [RegisterExternalAllocator](../../../api/graph_engine_api/cpp/ge/GeSession/RegisterExternalAllocator.md) | No change |
| [UnregisterExternalAllocator](../../../api/graph_engine_api/cpp/ge/Session/UnregisterExternalAllocator.md) | [UnregisterExternalAllocator](../../../api/graph_engine_api/cpp/ge/GeSession/UnregisterExternalAllocator.md) | No change |
| [IsGraphNeedRebuild](../../../api/graph_engine_api/cpp/ge/Session/IsGraphNeedRebuild.md) | [IsGraphNeedRebuild](../../../api/graph_engine_api/cpp/ge/GeSession/IsGraphNeedRebuild.md) | No change |
| [GetSessionId](../../../api/graph_engine_api/cpp/ge/Session/GetSessionId.md) | [GetSessionId](../../../api/graph_engine_api/cpp/ge/GeSession/GetSessionId.md) | No change |
| - | [GetCompiledModel](../../../api/graph_engine_api/cpp/ge/GeSession/GetCompiledModel.md) | New API: obtains the compiled model data. |
| [GetVariables](../../../api/graph_engine_api/cpp/ge/Session/GetVariables.md) | - | Deleted, no replacement API |
| [ShardGraphsToFile](../../../api/graph_engine_api/cpp/ge/Session/ShardGraphsToFile.md) | - | Deleted, the graph sharding feature is no longer provided. |
| [ShardGraphs](../../../api/graph_engine_api/cpp/ge/Session/ShardGraphs.md) | - | Deleted, the graph sharding feature is no longer provided. |
| [SaveGraphsToPb](../../../api/graph_engine_api/cpp/ge/Session/SaveGraphsToPb.md) | - | Deleted, the feature of saving graphs to **pb** files is no longer provided. |
| [PaRemapped](../../../api/graph_engine_api/cpp/ge/Session/PaRemapped.md) | - | Deleted, the virtual memory remapping feature is no longer provided. |

## Tensor Type Changes

The inputs/outputs of all runtime  APIs are changed from **ge::Tensor** to **gert::Tensor**. Details are as follows:

- Namespace changes

    |Feature|ge::Tensor|gert::Tensor|
    |--|--|--|
    |Namespace|ge|gert|
    |Data structure|Uses **std::shared_ptr\<TensorImpl\>** to manage the internal implementation.|Plain old data (POD) type, with all data stored inline|
    |Memory layout|Indirect access through the **impl_** pointer|Flattened layout, supporting direct memcpy|
    |Placement support|Sets placement through **TensorDesc**.|Supports multiple placement types.|
    |Copy behavior|Shallow copy (**shared_ptr** semantics)|Shallow copy, with pointer sharing|
    |Performance|Average|High|
    |Applicable scenario|Graph construction phase|Runtime execution|

- Data structure changes

  - Internal structure of **gert::Tensor**

    ```c++
    class Tensor {
    private:
      std::shared_ptr<TensorImpl> impl_;  // Managed by a smart pointer
    };
    ```

    - Use shared_ptr to manage TensorImpl.
    - Share the underlying implementation when copied.
    - Describe data using TensorDesc.

  - **ge::Tensor** internal structure

    ```c++
    class Tensor {
    private:
     StorageShape storage_shape_;        // Shape information
      StorageFormat storage_format_;      // Format information
      TensorVersion version_;             // Version
     uint8_t reserved_[3];               // Reserved field
     ge::DataType data_type_;            // Data type
     TensorData tensor_data_;            // Data pointer and placement
     uint8_t reserved_field_[40];        // Reserved field
    };
    ```

    - All fields are directly inlined in the object.
    - It is a standard-layout type (std::is_standard_layout).

The following describes the lifecycle of **gert::Tensor** and how to construct a **gert::Tensor**:

- Tensor Lifecycle<a id="li1"></a>
  - **RunGraph** API

    ```c++
    std::vector<gert::Tensor> inputs = ...;
    std::vector<gert::Tensor> outputs;
    session->RunGraph(graph_id, inputs, outputs);
    // Inputs and outputs can be safely released after the call completes.
    ```

  - **RunGraphWithStreamAsync** API

    ```c++
    // Note: CompileGraph and LoadGraph do not need to be called first; they are handled automatically.
    std::vector<gert::Tensor> inputs = ...;
    std::vector<gert::Tensor> outputs;
    session->RunGraphWithStreamAsync(graph_id, stream, inputs, outputs);
    // Inputs and outputs cannot be released before stream synchronization.
    // aclrtSynchronizeStream or another synchronization mechanism must be called.
    ```

  - **RunGraphAsync** API (important)

    ```c++
    using RunAsyncCallbackV2 = std::function<void(Status, std::vector<gert::Tensor>&)>;
    std::vector<gert::Tensor> inputs = ...;
    session->RunGraphAsync(graph_id, inputs,
        [](Status ret, std::vector<gert::Tensor>& outputs) {
            // Process the output.
        });
    // Important: inputs cannot be released immediately.
    // You must wait until the callback function is invoked before releasing inputs.
    // This is because the model reads inputs during execution, and the invocation of the callback ensures that model execution is complete.
    ```

- Methods for constructing **gert::Tensor**
  - Method 1: Construct using **TensorData** (recommended)

    ```c++
    #include "exe_graph/runtime/tensor.h"
    #include "acl_rt.h"

    // Construct a Host Tensor.
    void* host_buf = nullptr;
    aclError ret = aclrtMallocHost(&host_buf, data_len);  // Allocate Host memory.
    if (ret != ACL_ERROR_NONE) {
        // Handle the error.
    }

    // Construct using TensorData.
    gert::TensorData td(host_buf, nullptr, data_len, gert::kOnHost);
    gert::Tensor tensor;
    tensor.SetData(std::move(td));

    // Set the data type (if needed).
    // tensor.SetDataType(ge::DT_FLOAT);
    ```

  - Method 2: Construct directly using the constructor.

    ```c++
    // Construct from shape, format, and dtype.
    gert::StorageShape shape = {{batch_size, channels, height, width}, {4}};
    gert::StorageFormat format = {ge::FORMAT_ND, ge::FORMAT_ND, {}};
    gert::Tensor tensor(shape, format, ge::DT_FLOAT);

    // Then allocate memory.
    void* host_buf = nullptr;
    aclrtMallocHost(&host_buf, tensor.GetSize());
    gert::TensorData td(host_buf, nullptr, tensor.GetSize(), gert::kOnHost);
    tensor.SetData(std::move(td));
    ```

  - Method 3: Construct a device tensor.

    ```c++
    // Allocate device memory.
    void* dev = nullptr;
    aclError ret = aclrtMalloc(&dev, bytes, ACL_MEM_MALLOC_NORMAL_ONLY);
    if (ret != ACL_ERROR_NONE) {
        // Handle the error.
    }

    // Construct a device tensor.
    gert::TensorData td(dev, nullptr, bytes, gert::kOnDeviceHbm);
    gert::Tensor device_tensor;
    device_tensor.SetData(std::move(td));
    ```

- Free **gert::Tensor** memory.

    ```c++
    // Free Host Tensor memory.
    void FreeHostTensor(gert::Tensor &tensor) {
        if (tensor.GetAddr() != nullptr) {
            aclrtFreeHost(tensor.GetAddr());
        }
    }

    // Free Device Tensor memory.
    void FreeDeviceTensor(gert::Tensor &tensor) {
        if (tensor.GetAddr() != nullptr) {
            aclrtFree(tensor.GetAddr());
        }
    }

    // Release in batches.
    void FreeTensorVector(std::vector<gert::Tensor> &tensors, bool is_device) {
        for (auto &t : tensors) {
            if (t.GetAddr() != nullptr) {
                if (is_device) {
                    aclrtFree(t.GetAddr());
                } else {
                    aclrtFreeHost(t.GetAddr());
                }
            }
        }
    }
    ```

For details about APIs such as **aclrtMallocHost**, **aclrtMalloc**, **aclrtFreeHost**, and **aclrtFree**, see **Memory Management** in [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md).

## Compilation and Runtime API Changes

- **CompileGraph** and **LoadGraph** are no longer required.

    In GeSession, the three execution APIs RunGraph, RunGraphAsync, and RunGraphWithStreamAsync automatically check whether the graph has been compiled and loaded. If the graph has not been compiled, it is compiled automatically first; if it has not been loaded, it is loaded automatically first.

    ```c++
    // Automatic processing mechanism of GeSession
    GeSession session(options);
    session.AddGraph(graph_id, graph);

    // Execute directly without manually calling CompileGraph and LoadGraph.
    session.RunGraph(graph_id, inputs, outputs);  // Compile and load automatically
    ```

- Execution modes are mutually exclusive.

    The three execution modes of GeSession, RunGraph, RunGraphAsync, and RunGraphWithStreamAsync are mutually exclusive and cannot be mixed. Once an execution mode is used for a graph, the same mode must continue to be used for that graph.

## Compilation Configuration Changes

The **Makefile** or **CMakeLists.txt** needs to be updated:

|Session API|GeSession API|
|--|--|
|# Old configuration<br>target_link_libraries(your_app libge_runner.so)|# New configuration<br>target_link_libraries(your_app libge_runner_v2.so)|
