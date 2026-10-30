# Sample Code Analysis

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-21T09:19:45.303Z pushedAt=2026-10-28T03:30:34.468Z nl=2 -->

This practice adopts a modular design and is implemented in C++ by combining the GE graph engine APIs with the acl (Ascend Computing Language) APIs. To obtain the complete code, click [Recommendation Network High-Performance Example](../../../../../examples/recommendation). The core components are as follows:

1. **ModelInference::Builder**: A builder that configures model parameters. It encapsulates the construction process of the ModelInference object and provides a chained configuration interface.
2. **ModelInference**: The core class that provides core capabilities such as model initialization, resource management, and task scheduling.
3. **ModelInference::GraphWorker**: A worker thread that serves as the thread unit for executing asynchronous inference tasks.
4. **ModelInference::GraphTask**: A task unit that encapsulates the complete lifecycle of a single inference request (input/output and callback).

The following figure shows the UML (Unified Modeling Language) class diagram of each component:

**Figure 1** Sample code structure
![Figure 1 example](../figures/sample_code_structure.png "Sample code structure")

## API Call Process

The main execution process of the sample code and the APIs involved are as follows:

![Figure 2 Illustration](../figures/single_thread_14.png)

1. Call the "Initialization and Deinitialization \> aclInit" API in [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md) to initialize acl, and call the "Device Management \> aclrtSetDevice" API in **Runtime APIs** to specify the Device to run on.
2. Construct a ModelInference instance and initialize the feature switches: enable the batch H2D feature, configure the AICore core control policy, and use multi-instance parallelism.
3. Initialize ModelInference:
    1. Call the [Session](../../../api/graph_engine_api/cpp/ge/Session/Session.md) to create a Session class object and apply for Session resources. Configure the ge.aicoreNum parameter in the options of the Session.
    2. Call [GEInitialize](../../../api/graph_engine_api/cpp/ge/Session/GEInitialize.md) to perform system initialization.
    3. Call [aclgrphParseTensorFlow](../../../api/graph_engine_api/cpp/ge/aclgrphParseTensorFlow.md) to parse the model and obtain the Graph.
    4. Call [AddGraph](../../../api/graph_engine_api/cpp/ge/Session/AddGraph.md) to add the defined graph to the Session class object.
    5. Call [CompileGraph](../../../api/graph_engine_api/cpp/ge/Session/CompileGraph.md) to compile the graph.
    6. Call Device Management \> aclrtGetDevice in Runtime APIs to obtain the device to run.
    7. Create multiple threads, and pass the same Session, graph ID, and device ID to each thread.

4. Submit inference tasks to the worker threads. The following uses one thread as an example to describe the execution process of a worker thread:
    1. Call "Device Management \> aclrtSetDevice" in the **Runtime APIs** to specify the device to run, and call "aclrtCreateStream" to create a stream.
    2. Call [LoadGraph](../../../api/graph_engine_api/cpp/ge/Session/LoadGraph.md) (for the asynchronous graph execution scenario) to load the graph model onto the stream created in the previous step. Listen to the task queue to receive and execute tasks:
        1. Call "Memory Management \> aclrtMalloc" in the **Runtime APIs** to allocate device memory, and call "Memory Management \> aclrtMemcpyBatch" in the **Runtime APIs** to transfer data from the host to the device in batches. (If the batch H2D feature is enabled, use the aclrtMemcpyBatch API; otherwise, use the aclrtMemcpy API.)
        2. Call [ExecuteGraphWithStreamAsync](../../../api/graph_engine_api/cpp/ge/Session/ExecuteGraphWithStreamAsync.md) to run the graph asynchronously.
        3. Call "aclrtSynchronizeStream" to block the program until all tasks on the specified stream are complete.
        4. Call aclrtMemcpyBatch to transfer data from the device back to the host in batches.
        5. Call the memory management \> aclrtFree API in **Runtime APIs** to release memory.
        6. Execute the custom callback function.

5. Call [GEFinalize](../../../api/graph_engine_api/cpp/ge/Session/GEFinalize.md) to release system resources, and call the initialization and deinitialization \> aclFinalize API in **Runtime APIs** to release related resources.

## Development Example

1. Include the required header files, including those of acl, the C or C++ standard library, GE, and the sample ModelInference.

    ```c++
    #include <acl.h>
    #include <acl_rt.h>
    #include <sstream>
    #include <random>
    #include <unordered_map>
    #include <chrono>
    #include <atomic>
    #include <complex>
    #include <iostream>
    #include <vector>
    #include <map>
    #include "model_inference.h"
    #include <getopt.h>
    #include <string>
    ```

2. Initialize acl resources and set the Device.

    ```c++
    // Initialize acl
    aclError aerr = aclInit(nullptr);
    if (aerr != ACL_ERROR_NONE) {
      std::cerr << "Failed to init ACL, error=" << aerr << std::endl;
      return -1;
    }
    // Specify the Device used for computation
    aerr = aclrtSetDevice(0);
    if (aerr != ACL_ERROR_NONE) {
      std::cerr << "aclrtSetDevice failed, ret=" << aerr << std::endl;
      aclFinalize();
      return -1;
    }
    ```

3. Set the inference parameters.

    ```c++
    // Model file path
    const std::string model_path = "../data/DCN_v2.pb";
    // Model file type
    const std::string model_type = "TensorFlow";
    ```

4. Specify the model parsing parameters. The sample model has 27 input nodes.

    ```c++
    std::stringstream ss;
    // Define the number of input nodes
    int input_size = 27;
    for (int i = 1; i < input_size; ++i) ss << "Input_" << i << ":" << batchSize << ";";
    ss << "Input:" << batchSize << ",8";
    // Build a map to configure the model parsing parameters
    std::map<ge::AscendString, ge::AscendString> parser = {
        //  Set the output node
        {ge::AscendString(ge::ir_option::OUT_NODES),
         ge::AscendString("Identity:0")},
        // Set the input shape
        {ge::AscendString(ge::ir_option::INPUT_SHAPE),
         ge::AscendString(ss.str().c_str())}
    };
    ```

5. Build a ModelInference instance and initialize it.

    ```c++
    // Create a ModelInference instance
    auto model_inference = gerec::ModelInference::Builder(cfg.model_path, cfg.model_type)
                           .InputBatchCopy(enableBatchH2D)        // Enable the batch H2D feature
                           .AiCoreNum(aiCoreNum)                  // Configure AICore core control
                           .MultiInstanceNum(multiInstanceNum)    // Multi-instance parallelism
                           .GraphParserParams(cfg.parser_params)  // Set the graph parsing parameters
                           .Build();
    if (model_inference->Init() != ge::SUCCESS) {
      std::cerr << "Init ModelInference failed" << std::endl;
      return ge::FAILED;
    }
    ```

6. Submit the inference task.

    ```c++
    // Callback function used to perform cleanup and statistics after asynchronous inference completes.
    auto callback = [&](std::shared_ptr<std::vector<gert::Tensor>> outputs,
                        std::shared_ptr<std::vector<gert::Tensor>> inputs, bool status, long long exec_us) {
      if (status) {
        // If inference succeeds, update the success count and the total execution time.
        success_count.fetch_add(1, std::memory_order_relaxed);        // Increment the success count.
        total_exec_us.fetch_add(exec_us, std::memory_order_relaxed);  // Accumulate the execution time (in microseconds).
      }
      // Release the memory occupied by the output/input tensors.
      FreeHostTensors(outputs);
      FreeHostTensors(inputs);
    };

    // Execute multiple asynchronous inferences.
    for (int i = 0; i < num_runs; ++i) {
      if (model_inference->RunGraphAsync(all_inputs[i], all_outputs[i], callback) != ge::SUCCESS) {
        std::cerr << "RunGraphAsync failed at " << i << std::endl;
        return ge::FAILED;
      }
    }
    ```

    > [!NOTE]
    > The RunGraphAsync API uses an asynchronous execution mode and requires a callback function to be bound to process the inference results. The callback function must conform to the following signature:
    >
    >```c++
    >using Callback = std::function<void(
    >    std::shared_ptr<std::vector<gert::Tensor>> outputs,   // List of output tensors
    >    std::shared_ptr<std::vector<gert::Tensor>> inputs,    // List of input tensors
    >    bool status,                                          // Operation execution status
    >    long long exec_us                                     // Execution latency (in microseconds)
    >    )>;
    >```

7. Release resources.

    ```c++
    // Deinitialize ACL.
    aclFinalize();
    ```
