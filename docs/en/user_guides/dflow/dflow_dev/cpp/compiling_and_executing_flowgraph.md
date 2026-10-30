# Building and Running FlowGraph

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-28T12:58:00.328Z pushedAt=2026-10-29T04:44:10.089Z nl=2 -->

## Feature Overview

After the FlowGraph is built, if you want to directly compile and run the FlowGraph to obtain the graph execution result, refer to this section. The main APIs involved are as follows:

![](figures/260109163843623.png)

1. Call the "GEInitialize" API to perform system initialization (it can also be called before the graph is built) to request system resources.
2. Call the "Session constructor" to create a Session class object and request Session resources.
3. Call the "FlowGraph" API to add the defined graph to the Session class object.
4. Call the "RunGraph" API, or the "FeedDataFlowGraph (feed all inputs)" and "FetchDataFlowGraph (obtain all output data)" APIs, to run the graph.
5. Call the "GEFinalize" API to release system resources.

> [!NOTE]
> For details about "GEInitialize", "Session constructor", "AddGraph", "RunGraph", and "GEFinalize" in the preceding steps, see *[GE Graph Engine API](../../../../api/graph_engine_api/README.md)*.
> When the DataFlow development framework is used, the NN model is executed in saturation mode. In this mode, the computation precision may have errors. This mode is retained only for compatibility with earlier versions and will not evolve in the future.

## Development Example

1. Include the header files.

    ```cpp
    #include "ge_api.h"
    ```

2. Request system resources.

    After the graph is defined, call GEInitialize to perform system initialization (it can also be called before the graph is defined) to request system resources. The example code is as follows:

    ```cpp
    std::map<AscendString, AscendString>config = {{"ge.exec.deviceId", "0"},
                                                  {"ge.exec.logicalDeviceClusterDeployMode", "SINGLE"},
                                                  {"ge.exec.logicalDeviceId", "[0:0]"},
                                                  {"ge.graphRunMode", "1"},
                                                  {"ge.exec.precision_mode", "allow_fp32_to_fp16"}};
    Status ret = ge::GEInitialize(config);
    ```

    You can pass GE runtime initialization information through config. The parameters in the preceding configuration (ge.exec.deviceId, ge.graphRunMode, and ge.exec.precision\_mode) are used to specify the device on which the GE instance runs, the graph execution mode (set to 0 for online inference and 1 for training), and the operator precision mode, respectively.

    > [!NOTE]
    > Because some UDFs do not support load sharing, you need to add \{"ge.exec.logicalDeviceClusterDeployMode", "SINGLE"\}, \{"ge.exec.logicalDeviceId", "\[0:0\]"\} during GE initialization. logicalDeviceId can be \[0:0\] or \[0:1\]. If multi-instance deployment is required, see [Specifying the Deployment Location of the DataFlow Node](special_topics.md#specifying-the-deployment-location-of-the-dataflow-node) to implement multi-instance deployment. logicalDeviceId is described as follows.
    > When logicalDeviceClusterDeployMode is SINGLE, it specifies that the model is deployed on a specified device.
    > Configuration format: \[node\_id:device\_id\]
    >- node\_id: logical ID of the AI processor, starting from 0, indicating the ordinal position of the device in the resource configuration file.
    >- device\_id: physical ID of the AI processor.

3. Add a graph object and run the graph.

    To run the defined graph, first create a Session object, then call the AddGraph API to add the graph, and finally call the RunGraph API to execute the graph. The sample code is as follows:

    ```cpp
    std::map <AscendString, AscendString> options;
    ge::Session *session = new Session(options);
    if(session == nullptr) {
    std::cout << "Create session failed." << std::endl;
    return FAILED;
    }
    // Construct a FlowGraph.
    Status ret = session->AddGraph(graph_id, flow_graph.ToGeGraph());
    if(ret != SUCCESS) {
    return FAILED;
    }
    // Method 1: RunGraph
    ret = session->RunGraph(graph_id, input, output);
    if(ret != SUCCESS) {
    return FAILED;
    }
    // Method 2: FeedDataFlowGraph first, then FetchDataFlowGraph
    ge::DataFlowInfo dataFlowInfo;
    geRet = session->FeedDataFlowGraph(0, input, dataFlowInfo, 3000);
    if (geRet != ge::SUCCESS) {
        return FAILED;
    }
    geRet = session->FetchDataFlowGraph(0, output, dataFlowInfo, 3000);
    if (geRet != ge::SUCCESS) {
        return FAILED;
    }
    ```

    You can pass options to configure the graph running-related configuration information.

    The data obtained after the graph runs is stored in output.

    If the DataFlow graph contains UDF nodes, during graph compilation, DataFlow stores the compilation result in the workspace directory of the UDF specified by the user. The files written to disk are stored in the graph\_name\_release.tar.gz format. These files are used for decompression during the DataFlow model deployment phase. Ensure that these files are not manually deleted while the program is running.

4. After the graph runs, call GEFinalize to release resources.

    ```cpp
    ret = ge::GEFinalize();
    ```
