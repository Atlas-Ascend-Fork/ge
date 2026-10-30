# Building a Graph into an Offline Model

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-21T09:20:13.740Z pushedAt=2026-10-28T03:30:34.466Z nl=2 -->

This section describes the main APIs involved in building a graph into an om offline model and provides sample code for calling them.

## Feature Overview

The main APIs involved in building a graph into an offline model are as follows:

![Illustration](../figures/ir_graph_construction_2.png)

1. After the graph is defined, call [aclgrphBuildInitialize](../../../api/graph_engine_api/cpp/ge/aclgrphBuildInitialize.md) to initialize the system and apply for system resources.
2. Call [aclgrphBuildModel](../../../api/graph_engine_api/cpp/ge/aclgrphBuildModel.md) to build the graph into an offline model adapted to the AI processor. During compilation, the built-in operator library and custom operator library are loaded. At this point, the model is stored in the memory buffer.
3. (Optional) If you want to save the model in the memory buffer as an offline model file \*.om, call [aclgrphSaveModel](../../../api/graph_engine_api/cpp/ge/aclgrphSaveModel.md) to serialize and save the offline model to a file.
4. Call [aclgrphBuildFinalize](../../../api/graph_engine_api/cpp/ge/aclgrphBuildFinalize.md) to end the process and release system resources.

Currently, the model compilation and model file saving APIs can be called consecutively within a single process to build and save multiple offline models.

## Development Example

1. Include the header file.

    ```c++
    #include "ge_ir_build.h"
    #include "ge_api_types.h"
    ```

2. Apply for resources.

    After the graph is created, call [aclgrphBuildInitialize](../../../api/graph_engine_api/cpp/ge/aclgrphBuildInitialize.md) to initialize the system and apply for resources. The sample code is as follows:

    ```c++
    std::map<AscendString, AscendString> global_options = {
            {ge::ir_option::SOC_VERSION, "${soc_version}"},
        };
    auto status = aclgrphBuildInitialize(global_options);
    ```

    You can pass the **global\_options** parameter to configure the offline model compilation initialization information. For details about the supported configuration parameters, see [aclgrphBuildInitialize Configuration Parameters](../../../api/graph_engine_api/cpp/ge/aclgrphBuildInitialize_config_params/aclgrphbuildinitialize_config_params.md).

    If no AI processor is available in the current environment, SOC\_VERSION is a mandatory configuration used to specify the target chip version. Replace the value $\{soc\_version\} with the actual value. Configure other parameters as needed.

3. Call [aclgrphBuildModel](../../../api/graph_engine_api/cpp/ge/aclgrphBuildModel.md) to build the graph into an offline model. The sample code is as follows:

    ```c++
    ModelBufferData model;
    std::map<AscendString, AscendString> options;
    PrepareOptions(options);

    ge::Graph graph;
    status = aclgrphBuildModel(graph, options, model);
    if(status == GRAPH_SUCCESS) {
        cout << "Build Model SUCCESS!" << endl;
    }
    else {
        cout << "Build Model Failed!" << endl;
    }
    ```

    You can pass the **options** parameter to configure the offline model compilation information. For details about the supported configuration parameters, see [aclgrphBuildModel Configuration Parameters](../../../api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/aclgrphbuildmodel_config_params.md). Example configuration:

    ```c++
    void PrepareOptions(std::map<AscendString, AscendString>& options) {
        options.insert({
            {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, "1"} // close reuse memory
            });
    }
    ```

    > [!NOTE]
    >
    > When you call aclgrphBuildModel and pass the options parameter, in a multi-graph scenario, if the passed parameter is ge::ir\_option::PRECISION\_MODE or ge::ir\_option::PRECISION\_MODE\_V2, the parameter values set for multiple graphs must be the same.

4. (Optional) You can also call [aclgrphSaveModel](../../../api/graph_engine_api/cpp/ge/aclgrphSaveModel.md) to save the model in the memory buffer as an offline model file. The sample code is as follows:

    ```c++
    status = aclgrphSaveModel("ir_build_sample", model);
    if(status == GRAPH_SUCCESS) {
            cout << "Save Offline Model SUCCESS!" << endl;
    }
    else {
            cout << "Save Offline Model Failed!" << endl;
    }
    ```

5. When the graph construction process ends, release resources through the [aclgrphBuildFinalize](../../../api/graph_engine_api/cpp/ge/aclgrphBuildFinalize.md) API. The sample code is as follows:

    ```c++
    aclgrphBuildFinalize();
    ```

## Next Steps

- If the compilation above generates an om offline model:

    For subsequent inference, use the API for loading a model **from a file**, for example, aclmdlLoadFromFile, and then use aclmdlExecute to perform inference.

- If the offline model generated by the compilation above is stored in a memory buffer:

    For subsequent inference, use the API for loading a model **from memory**, for example, aclmdlLoadFromMem, and then use aclmdlExecute to perform inference.

For details about the APIs, see the "Model Inference" section in [Application Development](https://gitcode.com/cann/docs/blob/9.2.0/docs/zh/app-dev/c/00_acl_cpp_dev.md).
