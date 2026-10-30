# Running a Graph Synchronously

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-21T09:23:14.038Z pushedAt=2026-10-28T03:30:34.472Z nl=2 -->

This section describes the main APIs involved in synchronously running a graph and provides calling examples.

## Feature Overview

After a graph is built, if you want to **directly compile and run the graph to obtain the running result**, refer to this section. The main APIs involved are as follows. For a Session example of compiling and running a graph, see [graph\_run](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/8_graphrun/graph_run).

![Illustration](../figures/feature_intro_3.png)

1. Call [GEInitializeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEInitializeV2.md) to initialize the system (this can also be called before graph building) and apply for system resources.
2. Call the [GeSession](../../../api/graph_engine_api/cpp/ge/GeSession/GESession.md) to create a Session class object and apply for Session resources.
3. Call [AddGraph](../../../api/graph_engine_api/cpp/ge/GeSession/AddGraph.md) to add the defined graph to the Session class object.
4. Call [RunGraph](../../../api/graph_engine_api/cpp/ge/GeSession/RunGraph.md) to run the graph.
5. Call [GEFinalizeV2](../../../api/graph_engine_api/cpp/ge/GeSession/GEFinalizeV2.md) to release system resources.

## Development Example

1. Include the required header files.

    ```c++
    #include "ge_api_v2.h"
    ```

2. Apply for system resources.

    After the graph is defined, call GEInitializeV2 to initialize the system (this can also be done before the graph is defined) and apply for system resources. The sample code is as follows:

    ```c++
    std::map<AscendString, AscendString>config = {{"ge.exec.deviceId", "0"},
                                                  {"ge.graphRunMode", "1"}};
    Status ret = ge::GEInitializeV2(config);
    ```

    You can pass the initialization information for GE running through config. The configuration parameters ge.exec.deviceId and ge.graphRunMode specify the device on which the GE instance runs and the graph execution mode, respectively (set to 0 for online inference and 1 for training). For more configurations, see [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/options_parameters_description.md).

3. Add a graph object and run the graph.

    To run the defined graph, first create a Session object, then call the AddGraph API to add the graph, and finally call the RunGraph API to run the graph. The sample code is as follows:

    ```c++
    std::map<AscendString, AscendString>options;
    // Create a Session object
    ge::GeSession *session = new GeSession(options);
    // Check whether the Session is created successfully
    if(session == nullptr) {
      std::cout << "Create session failed." << std::endl;
      // ...
      // ...
      // Release resources
      ge::GEFinalizeV2();
      return FAILED;
    }
    // Prepare the graph ID to be added to the Session and create an empty graph object.
    uint32_t conv_graph_id = 0;
    ge::Graph conv_graph;

    // Add the graph to the session.
    Status ret = session->AddGraph(conv_graph_id, conv_graph);
    if(ret != SUCCESS) {
      // ...
      // ...
      // Release resources.
      ge::GEFinalizeV2();
      delete session;
      return FAILED;
    }
    // Prepare the input and output tensors.
    std::vector<gert::Tensor> input_cov;
    std::vector<gert::Tensor> output_cov;

    // Run the graph with the specified ID.
    ret = session->RunGraph(conv_graph_id, input_cov, output_cov);
    if(ret != SUCCESS) {
      // ...
      // ...
      // Release resources and destroy the Session.
      ge::GEFinalizeV2();
      delete session;
      return FAILED;
    }
    ```

    You can pass options to configure graph running-related information. For details about the related configuration, see [Session](../../../api/graph_engine_api/cpp/ge/GeSession/GESession.md).

    The data obtained after the graph runs is stored in the Tensor output\_cov.

4. After the graph runs, call GEFinalizeV2 to release resources.

    ```c++
    ret = ge::GEFinalizeV2();
    ```
