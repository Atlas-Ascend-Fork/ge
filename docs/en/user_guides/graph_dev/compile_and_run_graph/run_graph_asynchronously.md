# Running a Graph Asynchronously

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-21T09:22:15.952Z pushedAt=2026-10-28T03:30:34.477Z nl=2 -->

## Running Asynchronously on a Single Device in a Single Process

This section provides an example of running a graph using the asynchronous running APIs and device memory.

### Function Description

After the graph is built, if you want to **compile and asynchronously run the graph and obtain the running result of the graph**, refer to this section. The main APIs involved are as follows:

![Illustration](../figures/feature_intro_4.png)

1. Call [GEInitializeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEInitializeV2.md) to perform system initialization (which can also be called before the graph is built) and request system resources.
2. Call the "Initialization and Deinitialization \> aclInit" API in [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md) to initialize acl.
3. Call the [Session](../../../api/graph_engine_api/cpp/ge/GeSession/GESession.md) to create a Session class object and request Session resources.
4. Call the **Device Management** \> **aclrtSetDevice** API in [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md) to specify the device to run on, call **aclrtCreateStream** to create a stream, and then call the **Memory Management** \> **aclrtMallocHost/aclrtMalloc** API in [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md) to request host and device memory respectively.
5. Call [AddGraph](../../../api/graph_engine_api/cpp/ge/GeSession/AddGraph.md) to add the defined graph to the Session class object.
6. (Optional) Call [CompileGraph](../../../api/graph_engine_api/cpp/ge/GeSession/CompileGraph.md) to complete graph compilation.
7. (Optional) Call [LoadGraph](../../../api/graph_engine_api/cpp/ge/GeSession/LoadGraph.md) (in the asynchronous Graph execution scenario) to load the graph model onto the stream created in the preceding step.
8. Call the [RunGraphWithStreamAsync](../../../api/graph_engine_api/cpp/ge/GeSession/RunGraphWithStreamAsync.md) asynchronous running API to run the graph:

    If LoadGraph is not called to load the graph before this API is called, this API automatically calls LoadGraph to complete loading. If CompileGraph is not called to compile the graph before LoadGraph is called, LoadGraph automatically calls CompileGraph to complete compilation.

9. Call "aclrtSynchronizeStream" to block the program until all tasks in the specified stream are complete.
10. Call **aclrtFree** or **aclrtFreeHost** in **Memory Management** of **Runtime APIs** to free memory. Call [GEFinalizeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEFinalizeV2.md) to release system resources. Call **aclFinalize** in **Initialization and Deinitialization** of the **Runtime APIs** to release related resources.

### Development Example

1. Include the required header files, including the acl header files and the C or C++ standard library header files.

    ```c++
    #include "ge_api_v2.h"
    #include "acl.h"
    #include "acl_rt.h"
    ```

2. Request system resources.

    After the graph is defined, call GEInitializeV2 to perform system initialization (it can also be called before the graph is defined) to request system resources. The example code is as follows:

    ```c++
    std::map<AscendString, AscendString>config = {{"ge.exec.deviceId", "0"},
                                                  {"ge.graphRunMode", "1"}};
    Status ret = ge::GEInitializeV2(config);
    ```

    You can pass the initialization information for GE running through config. The configuration parameters ge.exec.deviceId and ge.graphRunMode specify the device on which the GE instance runs and the graph execution mode, respectively (set to 0 for online inference and 1 for training). For more configurations, see [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/options_parameters_description.md).

    > [!NOTE]
    >
    >It is recommended that the dump information in GE options and the dump information configured when calling the acl initialization API later not be configured at the same time. Otherwise, exceptions may occur. The same applies to other parameters with the same function.

3. Initialize acl resources.

    ```c++
    std::string aclConfigPath = "xx/xx/xx";
    aclError retInit = aclInit(aclConfigPath);
    if (retInit != ACL_ERROR_NONE) {
        // ...
        // ...
        return FAILED;
    }
    ```

4. Create a Session.

    To run the defined graph, first create a Session object. The options in the Session can load configuration parameters. For the supported configuration parameters, see [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/options_parameters_description.md).

    ```c++
    std::map <AscendString, AscendString> options;
    // Create a Session object
    ge::GeSession* session = new GeSession(options);
    // Check whether the Session is created successfully
    if(session == nullptr) {
      std::cout << "Create session failed." << std::endl;
      // ...
      // ...
      // Release resources
      ge::GEFinalizeV2();
      return FAILED;
    }
    ```

5. Specify the device to run on, create a stream, and request memory.

    ```c++
    // Specify the device for computation.
    int32_t deviceId = 0;
    retInit = aclrtSetDevice(deviceId);

    // Create a stream.
    aclrtStream stream = nullptr;
    aclError aclRet = aclrtCreateStream(&stream);

    // Request memory on the host.
    void* hostPtrA = NULL;
    size_t size = 1024;
    aclRet = aclrtMallocHost(&hostPtrA, size);
    // Request memory on the device.
    void* devPtrB = NULL;
    aclRet = aclrtMalloc(&devPtrB, size, ACL_MEM_MALLOC_HUGE_FIRST);

    // Copy memory to transfer data from the host to the device.
    // hostPtrA indicates the source memory address pointer on the host, devPtrB indicates the destination memory address pointer on the device, and size indicates the memory size.
    aclrtMemcpy(devPtrB, size, hostPtrA, size, ACL_MEMCPY_HOST_TO_DEVICE);

    ```

6. Add a graph object.

    Call the AddGraph API to add the graph. The example code is as follows:

    ```c++
    // Prepare the graph ID to be added to the Session and create an empty graph object
    uint32_t conv_graph_id = 0;
    ge::Graph conv_graph;
    // Add the Graph to the Session
    Status ret = session->AddGraph(conv_graph_id, conv_graph);
    if(ret != SUCCESS) {
      // ...
      // ...
      // Release resources and destroy the Session
      ge::GEFinalizeV2();
      delete session;
      return FAILED;
    }
    ```

    You can pass options to configure graph running-related configuration information. For details about the related configuration, see [Session](../../../api/graph_engine_api/cpp/ge/GeSession/GESession.md). The data generated after the graph runs is stored in the Tensor output\_cov.

7. (Optional) Compile the graph.

    If the CompileGraph API is not called, the LoadGraph API automatically calls CompileGraph to complete compilation.

    ```c++
    uint32_t graph_id = 0;
    ret = session->CompileGraph(graph_id);
    if(ret != SUCCESS) {
      // ...
      // ...
      // Release resources
      ge::GEFinalizeV2();
      delete session;
      return FAILED;
    }
    ```

8. (Optional) Load the graph to the created stream.

    If the LoadGraph API is not called, the RunGraphWithStreamAsync API automatically calls LoadGraph to complete loading. The options in LoadGraph can load configuration parameters, for example, **"ge.exec.frozenInputIndexes (input Tensor indexes whose addresses are not refreshed)"**. This configuration parameter can improve graph execution performance. For details about the parameters, see [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/basic_functions.md).

    ```c++
    std::map <AscendString, AscendString> options;
    uint32_t graph_id = 0;
    ret = session->LoadGraph(graph_id, options, stream);
    if(ret != SUCCESS) {
      // ...
      // ...
      // Release resources
      ge::GEFinalizeV2();
      delete session;
      return FAILED;
    }
    ```

9. Data transfer.

    ```c++
    // Copy memory to transfer data from the host to the device
    // hostPtrA indicates the source memory address pointer on the host, devPtrB indicates the destination memory address pointer on the device, and size indicates the memory size
    aclrtMemcpy(devPtrB, size, hostPtrA, size, ACL_MEMCPY_HOST_TO_DEVICE);
    ```

10. Execute the graph asynchronously and output the execution result.

    ```c++
    ret = session->RunGraphWithStreamAsync(graph_id, stream, input, output);

    // Call aclrtSynchronizeStream to block the application until all tasks in the specified stream are complete.
    aclRet = aclrtSynchronizeStream(stream);

    // Copy memory to transfer data from the device back to the host.
    // devPtrA indicates the source memory address pointer on the device, hostPtrB indicates the destination memory address pointer on the host, and size indicates the memory size.
    aclrtMemcpy(hostPtrB, size, devPtrA, size, ACL_MEMCPY_DEVICE_TO_HOST);

    ```

11. Release resources.

    ```c++
    // Free memory.
    ret = aclrtFree(devPtrB);
    ret = aclrtFreeHost(hostPtrA);

    // Release graph resources.
    ret = ge::GEFinalizeV2();

    // Deinitialize ACL.
    ret = aclFinalize();
    ```

## Running Asynchronously on Multiple Devices in a Single Process

This section describes how to manage multiple devices in a single process, that is, a process can execute concurrently on different devices.

### Function Description

The main APIs involved are as follows:

![Illustration](../figures/single_thread.png)

1. Call [GEInitializeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEInitializeV2.md) to perform system initialization (which can also be called before graph construction) and request system resources.
2. Call the **Initialization and Deinitialization** \> **aclInit** API in [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md) to initialize acl.
3. Call the [Session](../../../api/graph_engine_api/cpp/ge/GeSession/GESession.md) to create multiple Session class objects and request Session resources. Pass a different ge.session\_device\_id to each Session to run the model on different devices.
4. Create multiple threads, each receiving a different Session. The following uses one thread as an example to describe the simple process:
    1. Call "Device Management \> aclrtSetDevice" in [Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md) to specify the device to run on, call "aclrtCreateStream" to create a stream, and then call "Memory Management \> aclrtMalloc" in *[Runtime APIs](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md)* to request device memory.
    2. Call [AddGraph](../../../api/graph_engine_api/cpp/ge/GeSession/AddGraph.md) to add the defined graph to the Session class object.
    3. Call [CompileGraph](../../../api/graph_engine_api/cpp/ge/GeSession/CompileGraph.md) to complete graph compilation.
    4. Call [LoadGraph](../../../api/graph_engine_api/cpp/ge/GeSession/LoadGraph.md) (for the asynchronous execution graph scenario) to load the graph model onto the stream created earlier.
    5. Call "memory management \> aclrtMemcpy" in the **Runtime APIs** to transfer data from the host to the device.
    6. Call the [RunGraphWithStreamAsync](../../../api/graph_engine_api/cpp/ge/GeSession/RunGraphWithStreamAsync.md) asynchronous execution API to run the graph.
    7. Call "aclrtSynchronizeStream" to block the program until all tasks on the specified stream are complete.
    8. Call "memory management \> aclrtMemcpy" in the **Runtime APIs** to transfer data from the device back to the host.
    9. Call "memory management \> aclrtFree" in the **Runtime APIs** to free memory.

5. Call [GEFinalizeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEFinalizeV2.md) to release system resources; call "initialization and deinitialization \> aclFinalize" in the **Runtime APIs** to release related resources.

### Development Example

1. Include the required header files, including the acl header files and the C or C++ standard library header files.

    ```c++
    #include "ge_api_v2.h"
    #include "acl.h"
    #include "acl_rt.h"
    #include "graph/ascend_string.h"
    #include <thread>
    ```

2. Request system resources.

    After the graph is defined, call GEInitializeV2 to perform system initialization (it can also be called before the graph is defined) and request system resources. The example code is as follows:

    ```c++
    std::map<AscendString, AscendString>config = {{"ge.exec.deviceId", "0"},
                                                  {"ge.graphRunMode", "1"}};
    Status ret = ge::GEInitializeV2(config);
    ```

    You can pass the initialization information for GE running through config. The configuration parameters ge.exec.deviceId and ge.graphRunMode specify the device on which the GE instance runs and the graph execution mode, respectively (set to 0 for online inference and 1 for training). For more configurations, see [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/basic_functions.md).

    > [!NOTE]
    >
    >It is recommended that the dump information in GE options and the dump information configured when calling the acl initialization API later not be configured at the same time. Otherwise, exceptions may occur. The same applies to other parameters with the same function.

3. Initialize acl resources.

    ```c++
    std::string aclConfigPath = "xx/xx/xx";
    aclError retInit = aclInit(aclConfigPath);
    if (retInit != ACL_ERROR_NONE) {
        // ...
        // ...
        return FAILED;
    }
    ```

4. Create multiple Sessions.

    To run a defined graph, first create a Session object. The options in the Session can load configuration parameters. For the supported configuration parameters, see [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/options_parameters_description.md).

    ```c++
    int thread_num = 8; // Take 8 devices as an example
    for (int i= 0; i < thread_num; ++i) {   // Create multiple Sessions, and pass a different ge.session_device_id in the options of each Session.
        std::map<ge::AscendString, ge::AscendString> options = { // Construct the Session configuration.
     {"ge.session_device_id",std::to_string(i).c_str()},
        };
        ge::GeSession *session = new ge::GeSession(options); // Create a Session and pass in the configuration map.
        if (session == nullptr) { // Check whether the Session is created successfully.
     std::cout << "create session failed!" << std::endl;
     ge::GEFinalizeV2();
     return FAILED;
        }
        sessions.push_back(session);
    }
    ```

5. Create multiple threads, pass a different Session and ge.session\_device\_id to each thread, and run the graph asynchronously.

    ```c++
    // Container for storing all thread objects
    std::vector<std::thread> threads;
    // Create multiple threads and store them in the container.
    for (int i= 0; i < thread_num; i++) {
        std::thread worker_thread(exec_func, i); // Create a thread to execute exec_func. The exec_func thread function is shown in steps 5.a to 5.g.
        threads.emplace_back(std::move(worker_thread)); // Move worker_thread into the threads container using std::move.
    }
    // Wait for all threads to complete.
    for (int i = 0; i < thread_num; i++) {
        threads.at(i).join();
    }
    ```

    The steps for asynchronous running of a single thread exec\_func are as follows:

    1. Specify the device for running, create a stream, and request memory.

        ```c++
        // Specify the device for computation.
        int32_t deviceId = 0;
        retInit = aclrtSetDevice(deviceId);

        // Create a stream.
        aclrtStream stream = nullptr;
        aclError aclRet = aclrtCreateStreamWithConfig(&stream, 0, ACL_STREAM_FAST_LAUNCH);

        // Request memory on the device.
        void* devPtrB = NULL;
        aclRet = aclrtMalloc(&devPtrB, data_size, ACL_MEM_MALLOC_HUGE_FIRST);
        ```

    2. Add a graph object.

        ```c++
        uint32_t graph_id = 0;
        ge::Graph graph;
        sess_ = sessionList[index];
        ge::Status ret = sess_ -> AddGraph(graph_id, graph, graph_options);
        if(ret != SUCCESS) {
          // ...
          // ...
          // Release resources
          ge::GEFinalizeV2();
          delete session;
          return FAILED;
        }
        ```

        You can pass in options to configure graph running-related information, where the data generated after the graph runs is stored in the Tensor output\_cov.

    3. (Optional) Compile the graph.

        If the CompileGraph API is not called, the LoadGraph API automatically calls CompileGraph to complete compilation.

        ```c++
        uint32_t graph_id = 0;
        ret = sess_ -> CompileGraph(graph_id);
        if(ret != SUCCESS) {
          // ...
          // ...
          // Release resources and destroy the Session
          ge::GEFinalizeV2();
          delete session;
          return FAILED;
        }
        ```

    4. (Optional) Load the graph to the created stream.

        If the LoadGraph API is not called, the RunGraphWithStreamAsync API automatically calls LoadGraph to complete loading. The options in LoadGraph can load configuration parameters. For details about the supported configuration parameters, see [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/options_parameters_description.md).

        ```c++
        std::map <AscendString, AscendString> options;
        uint32_t graph_id = 0;
        ret = sess_ -> LoadGraph(graph_id, options, stream);
        if(ret != SUCCESS) {
          // ...
          // ...
          // Release resources and destroy the Session
          ge::GEFinalizeV2();
          delete session;
          return FAILED;
        }
        ```

    5. Transfer data.

        ```c++
        // Copy memory to transfer data from the host to the device.
        // hostPtrA indicates the source memory address pointer on the host, devPtrB indicates the destination memory address pointer on the device, and size indicates the memory size.
        aclrtMemcpy(devPtrB, size, hostPtrA, size, ACL_MEMCPY_HOST_TO_DEVICE);
        ```

    6. Run the graph asynchronously and output the execution result.

        ```c++
        std::vector<gert::Tensor> input;
        std::vector<gert::Tensor> output;
        ret = sess_->RunGraphWithStreamAsync(graph_id, stream, input, output);

        // Call the aclrtSynchronizeStream API to block the application until all tasks in the specified stream are complete.
        aclRet = aclrtSynchronizeStream(stream);
        // Copy memory to transfer data from the device back to the host.
        // devPtrA indicates the source memory address pointer on the device, hostPtrB indicates the destination memory address pointer on the host, and size indicates the memory size.
        aclrtMemcpy(hostPtrB, size, devPtrA, size, ACL_MEMCPY_DEVICE_TO_HOST);
        ```

    7. Free memory.

        ```c++
        // Free memory
        ret = aclrtFree(devPtrB);
        ```

6. Release resources.

    ```c++
    // Release the resources of each session
    for (auto session : sessions) {
        delete session;
    }

    // Release the graph resources
    ret = ge::GEFinalizeV2();

    // Deinitialize ACL
    ret = aclFinalize();
    ```
