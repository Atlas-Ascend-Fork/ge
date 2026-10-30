# Custom Concurrency Using CustomStreamPassFunc

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-21T09:46:45.331Z pushedAt=2026-10-28T06:35:24.761Z nl=2 -->

## Feature Overview

Due to runtime factors such as operator execution time, core occupancy, and bandwidth, the same algorithm may achieve different concurrency on different hardware, and these effects cannot be accurately estimated at the compilation stage. Currently, no perfect algorithm exists that can achieve optimal concurrency on any network and any hardware.

To address the preceding issues, this section introduces the capability of custom logical stream allocation Pass. You can analyze the hardware utilization of a specific network on specific hardware based on the graph topology and Profiling, so as to flexibly adjust the concurrency effect with one policy for one network and one card, achieving optimal performance. Custom logical stream allocation Pass is typically used in the following two scenarios:

- Fine-tuning the built-in logical stream allocation result.
- Developing a self-developed stream allocation algorithm based on the graph structure.

Related concepts:

- Logical stream: A concept corresponding to physical stream. It is a stream pre-allocated on the graph based on certain conditions (topological order and engine ownership). Tasks on this logical stream are executed in sequence. All streams described in this document are logical streams.
- Stream allocation: Specifying that certain tasks can be executed in parallel, which improves hardware utilization and reduces model execution time.

This section uses scenario 1 "Fine-tuning the built-in logical stream allocation result" as an example to describe in detail how to use a custom logical stream allocation Pass to customize concurrency. The overall process is as follows:

First, you can call the [REGISTER\_CUSTOM\_PASS](../../../api/graph_engine_api/cpp/ge/REGISTER_CUSTOM_PASS.md) registration macro to register the Pass with a specified Pass name. Then, by compiling the function into a dynamic library plugin, the registered Pass is invoked after the logical stream allocation stage. Finally, you can fine-tune the built-in stream allocation result. The following is an example:

```c++
#include "register_custom_pass.h"
// User-defined logical stream allocation function
Status CustomStreamPassFunc(const ConstGraphPtr &graph, StreamPassContext &stream_context) {
    // Define the logical stream allocation behavior in this function.
    return SUCCESS;
}
// Pass registration. No stage needs to be specified. By default, the Pass is executed after the logical stream allocation stage.
REGISTER_CUSTOM_PASS("pass_name").CustomAllocateStreamPassFn(CustomStreamPassFunc);
```

- register\_custom\_pass.h: stored in the "CANN software installation directory/cann/include/register/" directory. Include this header file to use Pass registration-related classes and Pass registration-related APIs.
- Status: ge::GRAPH\_SUCCESS is returned on success, and any other return value indicates failure. It is recommended to use a value less than 0 as the returned error code. A value greater than 0 may conflict with error codes already used by the framework.
- CustomStreamPassFunc: the execution function of the custom Pass. For details, see [Callback Function CustomAllocateStreamPassFunc](../../../api/graph_engine_api/cpp/ge/PassRegistrationData/CustomAllocateStreamPassFn.md#callback-function-customallocatestreampassfunc).
- graph: the graph for which logical streams are to be allocated. The type is ConstGraphPtr.
- stream\_context: a StreamPassContext class object. For details, see the methods provided by [StreamPassContext](../../../api/graph_engine_api/cpp/ge/StreamPassContext/overview.md).
- REGISTER\_CUSTOM\_PASS: registers a custom Pass. "pass\_name" can be named arbitrarily. For details, see [REGISTER\_CUSTOM\_PASS](../../../api/graph_engine_api/cpp/ge/REGISTER_CUSTOM_PASS.md).
- **CustomAllocateStreamPassFn**: registers the execution function of the custom logical stream allocation Pass. For details, see [CustomAllocateStreamPassFn](../../../api/graph_engine_api/cpp/ge/PassRegistrationData/CustomAllocateStreamPassFn.md).

> [!NOTE]Note
>
>When the forced single stream switch is enabled (ge.enableSingleStream is set to true in [Options Parameter Description](../../../api/graph_engine_api/cpp/ge/options_params/basic_functions.md)), the custom logical stream allocation Pass will not be executed.

## Development Example

- **Prerequisites**

    Combine the following diagram structure and Profiling analysis to identify optimization points:

    **Figure 1**  Original graph<a id="fig1"></a>
    ![Illustration](../figures/original_image.png "Original graph")

    The Profiling analysis result is shown in the following figure. According to the Profiling analysis result, operators 1 and 2, which **meet the concurrency condition (no data dependency or control dependency)**, are executed serially (after logical stream allocation, operators 1 and 2 are assigned the same Stream ID). You can use the feature provided in this section to change them to parallel execution.

    ![Illustration](../figures/fusion_pass_image7.png)

    >[!NOTE]
    >
    >If operators that meet the concurrency condition use the same computing resources and resource preemption and waiting exist, concurrency benefits are not always obtained. A detailed Profiling analysis is required. This example is used only to describe how to make the adjustment.

- **Procedure**
    1. Include the following header file:

        ```c++
        #include <iostream>
        // Custom Pass API header file
        #include "register_custom_pass.h"
        ```

    2. Develop a custom Pass to allocate a new stream ID to node 1. (The following code is only an example and cannot be executed.)

        Note: After obtaining the graph of the logical flow to be allocated, you can only modify the nodes on the current graph. That is, you can only use the [GetDirectNode](../../../api/graph_engine_api/cpp/ge/Graph/GetDirectNode.md) API to obtain all nodes in this graph.

        ```c++
        #include <iostream>
        #include "register_custom_pass.h"
        // Custom stream allocation Pass
        graphStatus AllocateStreamPass(const ConstGraphPtr &graph, StreamPassContext &context) {
            // Traverse all direct child nodes in the graph (that is, do not recurse into subgraphs)
            for (const auto &node : graph->GetDirectNode()) {
                AscendString node_name;
                node.GetName(node_name); // Obtain the node name
                // Check whether the node is the specified Abs_1 node
                if (std::string(node_name.GetString()) == "Abs_1/unary_ops_composition_0_SquareAbs_1/unary_ops_composition_1_Abs") {
                    // Allocate a new stream ID to this node
                    context.SetStreamId(node, context.AllocateNextStreamId());
                }
            }
            return GRAPH_SUCCESS;
        }
        // Register the custom Pass
        REGISTER_CUSTOM_PASS("AllocateStreamPass").CustomAllocateStreamPassFn(AllocateStreamPass);
        ```

## How to Use a Custom Pass

After completing the custom Pass described above, this section briefly describes how to compile the user-defined logical stream allocation function into a dynamic library plugin so that the registered Pass is invoked after the logical stream allocation phase.

- **Prerequisites**

    Install the CANN software package by referring to [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).

- **Program Compilation:**
    1. See [Sample Usage Guide](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/3_ir/2_fuse_matmul_add_pass) to obtain the CMakeLists.txt editing script, and place the user-defined logical stream allocation function **_AllocateStreamPass.cpp_** file in the src directory according to the directory structure in the sample.
    2. Modify the following information in the CMakeLists.txt file based on the actual situation:
        - ASCEND\_PATH: Specify the storage path of the CANN software after installation. For example, for the root installation user, the path is /usr/local/Ascend/cann.
        - target\_include\_directories: Header files to be included. For this example, no modification is required. If the code is developed by the user and header files need to be added, add lines directly below the example. Do not delete the original items. If the network contains custom operators, add the prototype definition header file of the custom operators.
        - target\_link\_libraries: Libraries to be linked. For this example, no modification is required. If the code is developed by the user and link libraries need to be added, add lines directly below the example. Do not delete the original items.

    3. Run the following command to compile:

        ```bash
        mkdir build && cd build
        cmake .. && make
        ```

        After compilation, the dynamic library file **lib_AllocateStreamPass_.so** is generated in the build directory.

    4. Copy lib_AllocateStreamPass_.so to the $\{ASCEND\_PATH\}/opp/vendors/_xxx_/custom\_fusion\_passes/ directory, where "_xxx_" is a user-defined directory. (Soft links are supported. The ".so" file must be readable by the user who executes it.)

        Multiple "$\{ASCEND\_PATH\}/opp/vendors/_xxx_" directories are sorted in text order and then traversed to find the "custom\_fusion\_passes/" subdirectory. Within a single subdirectory, ".so" files are loaded in text order, and files not ending with ".so" are skipped during loading:

        - _xxx_: There is exactly one level of custom directory.
        - custom\_fusion\_passes: This directory cannot contain subdirectories.

- **Using the custom Pass** (the following entry points for compiling model files are supported but not limited to these)

    To check whether the custom Pass takes effect, dump the graph before compiling the model: set the [DUMP\_GE\_GRAPH](../../../user_guides/env_vars/DUMP_GE_GRAPH.md) environment variable before execution, and then use the following entry points to compile the model:

  - Use the ATC tool for model conversion. For details about how to use the ATC tool, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).
  - [Building a Graph into an Offline Model](../compile_and_run_graph/compile_graph_to_offline_model.md).
  - [Building and Running a Graph](../compile_and_run_graph/compile_and_run_graph.md).

## Result Verification

> [!NOTE]
>
> If a dynamic shape model contains sinkable parts, the framework internally splits the model into two parts: dynamic scheduling and sink scheduling (static subgraph), where the sink scheduling may contain multiple small models. Therefore, when performing custom stream allocation on a dynamic-static hybrid model, there will be multiple dump graphs before and after the custom Pass: the first one corresponds to the root graph, and the subsequent ones correspond to the static models in the model. When you want to view the allocation result through the dump graphs, it is best to view the dump graph of the last custom stream allocation before the build graph.

After the dump environment variables are set and the program execution is complete, graph files such as ge\_onnx\*.pbtxt are generated in the current path. You can obtain the following two graphs and view them using visualization software such as Netron:

- ge\_onnx\__xxx_\_RunCustomPass\_BeforeAssignLogicStream\*.pbtxt: the graph before the Pass is executed, where \* represents the specific Pass name. For the graph structure, see [Figure 1](#fig1).
- ge\_onnx\__xxx_\_RunCustomPass\_AfterAssignLogicStream\*.pbtxt: the graph after the Pass is executed, where \* represents the specific Pass name. The graph structure is as follows:

    ![Illustration](../figures/fusion_pass_image8.png)

    As shown in the figure, operator 1 is assigned a new Stream ID.

You can also view the final result based on the compiled graph of the model in combination with Profiling: as shown in [Figure 2](#fig2), inter-stream synchronization operators such as Send/Recv are generated before and after operator 1. After Profiling is generated ([Figure 3](#fig3)), you can see that operators 1 and 2 are executed concurrently on two streams. For details about Profiling, see [Profiling Data Collection](../more_features/profiling_data_collection.md).

**Figure 2** Compiled graph<a id="fig2"></a>
![Illustration](../figures/compiled_graph.png "Compiled graph")

**Figure 3** Profiling result<a id="fig3"></a>
![Illustration](../figures/profiling_result.png "Profiling result")
