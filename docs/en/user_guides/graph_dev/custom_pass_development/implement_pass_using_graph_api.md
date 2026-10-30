# Implementing Pass Based on the Graph Modification API

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-21T09:47:52.499Z pushedAt=2026-10-28T07:19:10.798Z nl=2 -->

This section describes how to modify a Graph by using a custom Pass encapsulated through the graph modification API.

The Graph modification feature supports two calling methods:

- Direct API call: After a Graph is built, you can directly call the graph modification API (for details, see [Modifying a Graph Using the Graph Modification API](../construct_graph/modify_graph_using_graph_api.md)) to adjust the graph structure in real time. This method is suitable for simple, one-time modification scenarios.
- Custom Pass method: A more recommended method is to encapsulate the graph modification function as a custom Pass and register it through the [REGISTER\_CUSTOM\_PASS](../../../api/graph_engine_api/cpp/ge/REGISTER_CUSTOM_PASS.md) macro. The registered Pass can be compiled into a dynamic library plugin and invoked at a specified phase of the graph optimization pipeline, enabling modular, reusable, and configurable graph transformation capabilities.

This chapter describes in detail how to develop, register, and integrate a custom Pass, helping you implement efficient and flexible Graph optimization strategies.

## Feature Overview

A graph modification function can be regarded as a custom Pass. You can call the REGISTER\_CUSTOM\_PASS registration macro to register the Pass under a specified Pass name. By compiling the graph modification function into a dynamic library plugin, the registered Pass is invoked at the specified phase. The following is an example:

```c++
#include "register_custom_pass.h"
// User-defined graph modification function
graphStatus CustomPassFunc(GraphPtr &graph, CustomPassContext &custom_context) {
    // Define the specific graph modification behavior here
    return GRAPH_SUCCESS;
}
// Register the graph modification Pass. The registered Pass is invoked at the specified phase.
REGISTER_CUSTOM_PASS("pass_name").CustomPassFn(CustomPassFunc).Stage(CustomPassStage::kBeforeInferShape);
```

- register\_custom\_pass.h: Stored in the "CANN software installation directory/cann/include/register/" directory. Include this header file to use the Pass registration-related classes and Pass registration-related APIs.
- graphStatus: Returns ge::GRAPH\_SUCCESS on success. Any other return value indicates failure. It is recommended to use a value less than 0 as the error code. A value greater than 0 may conflict with error codes already used by the framework.
- CustomPassFunc: Entry function of the custom Pass. For details, see [Callback Function CustomPassFunc](../../../api/graph_engine_api/cpp/ge/PassRegistrationData/CustomPassFn.md).
- graph: Graph to be modified by the custom Pass. The type is GraphPtr.
- custom\_context: Object of the CustomPassContext class. For details, see the methods provided by [CustomPassContext](../../../api/graph_engine_api/cpp/ge/CustomPassContext/CustomPassContext.md).
- REGISTER\_CUSTOM\_PASS: Registers a custom Pass. "pass\_name" can be named arbitrarily. For details, see [REGISTER\_CUSTOM\_PASS](../../../api/graph_engine_api/cpp/ge/REGISTER_CUSTOM_PASS.md).
- CustomPassFn: the execution function of the custom Pass. For details, see [CustomPassFn](../../../api/graph_engine_api/cpp/ge/PassRegistrationData/CustomPassFn.md).
- Stage: specifies the execution phase of the Pass. For details, see [Stage](../../../api/graph_engine_api/cpp/ge/PassRegistrationData/Stage.md).

> [!NOTE]
>
> If you need to replace an operator with one that provides other functions during graph modification, but the operator is not supported by CANN, you can customize the operator in the following ways:
>
> Customize the operator through Ascend C. For details, see [Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md).
>
> Customize the operator through TBE. For details, see *TBE & AI CPU Operator Development*.
>
> Only after the operator is developed can the custom Pass be used normally to modify the Graph.

## Development Example

This section uses the fusion of the MatMul+Add structure into a GEMM custom pass as an example to describe in detail how to modify a graph through a custom pass. For details, see [sample source code](https://gitee.com/ascend/samples/blob/master/cplusplus/level1_single_api/3_ir/2_fuse_matmul_add_pass/src/fuse_matmul_add_pass.cpp).

The sample repository also provides the following two samples that use custom passes to modify graphs. You can click the corresponding links to view them:

- How to modify the Tile+Concat graph structure into the Concat+Tile+Concat graph structure through a custom pass. Click [fuse\_tile\_concat\_pass](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/3_ir/1_fuse_tile_concat_pass) to obtain the link.
- How to use a custom pass to modify a subgraph by inserting an Abs node between the Data and FrameworkOp structures of the subgraph. Click [modify\_subgraph\_pass](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/3_ir/3_modify_subgraph_pass) to obtain the sample.

1. Include the header files.

    ```c++
    #include <iostream>
    // Header file of the custom pass API
    #include "register_custom_pass.h"
    // Header file of the newly added operator
    #include "all_ops.h"
    // If an operator is customized using Ascend C, include the following header file (if Ascend C custom operators are not supported, ignore the following header file):
    #include "<CANN software installation directory>/cann/opp/vendors/<customize>/op_proto/inc/op_proto.h"
    ```

    Where, modify \<CANN software installation directory\> to the actual installation path of the CANN software package. <customize\>: modify it to the actual project name of the Ascend C custom operator.

2. Use a custom Pass to modify the Graph. (The following code is only an example and cannot be executed. When modifying the Graph, you can only use the APIs in [CustomPassContext](../../../api/graph_engine_api/cpp/ge/CustomPassContext/CustomPassContext.md), [Graph](../../../api/graph_engine_api/cpp/ge/Graph/Graph.md), [GNode](../../../api/graph_engine_api/cpp/ge/GNode/GNode.md), [PassReceiver](../../../api/graph_engine_api/cpp/ge/PassReceiver/PassReceiver.md), [PassRegistrationData](../../../api/graph_engine_api/cpp/ge/PassRegistrationData/PassRegistrationData.md), [REGISTER\_CUSTOM\_PASS](../../../api/graph_engine_api/cpp/ge/REGISTER_CUSTOM_PASS.md), and [StreamPassContext](../../../api/graph_engine_api/cpp/ge/StreamPassContext/overview.md).)

    ```c++
    namespace {
    constexpr const char *kOpNameAdd = "add";
    constexpr const char *kOpNameMatMul = "matmul";
    constexpr const char *kOpNameGEMM = "gemm";
    constexpr const char *kOpNameAlpha = "alpha";
    constexpr const char *kOpNameBeta = "beta";
    constexpr const char *kAttrNameTransposeA = "transpose_a";
    constexpr const char *kAttrNameTransposeB = "transpose_b";
    constexpr int32_t kIndex0 = 0;
    constexpr int32_t kIndex1 = 1;
    constexpr int32_t kIndex2 = 2;
    constexpr int32_t kIndex3 = 3;
    constexpr int32_t kIndex4 = 4;

    // 1. Traverse all nodes to find MatMul and Add nodes.
    bool FindNodes(GraphPtr &graph, GNode &src_node, GNode &dst_node) {
        auto all_nodes = graph->GetAllNodes();
        bool find_src_node = false;
        bool find_dst_node = false;
        for (auto &node: all_nodes) {
            AscendString node_name;
            auto ret = node.GetName(node_name);
            // Find MatMul.
            if (node_name == kOpNameMatMul) {
                src_node = node;
                find_src_node = true;
                cout << "Find src node: MatMul." << endl;
            // Find Add.
            } else if (node_name == kOpNameAdd) {
                dst_node = node;
                find_dst_node = true;
                cout << "Find dst node: Add." << endl;
            }
        }
        return (find_src_node && find_dst_node);
    }
    // 2. Determine whether the MatMul and Add nodes have an edge connection.
    bool CheckNodesHaveEdge(GraphPtr &graph, const GNode &src_node, const GNode &dst_node) {
        for (auto &[out_node, _]: src_node.GetOutDataNodesAndPortIndexs(kIndex0)) {
            AscendString node_name;
            auto ret = out_node->GetName(node_name);
            if (node_name == kOpNameAdd) {
                return true; // Directly connected.
            }
        }
        return false;
    }
    // 3. Create and add the GEMM node.
    void CreateGEMMNode(GraphPtr &graph, const GNode &src_node, GNode &node_gemm) {
        bool transpose_a = false;
        bool transpose_b = false;
        src_node.GetAttr(kAttrNameTransposeA, transpose_a);
        src_node.GetAttr(kAttrNameTransposeB, transpose_b);
        constexpr float kValue1 = 1;
        TensorDesc alpha_desc(ge::Shape({1}), FORMAT_ND, DT_FLOAT);
        Tensor alpha_tensor(alpha_desc, reinterpret_cast<const uint8_t *>(&kValue1), sizeof(float));
        auto alpha = op::Const(kOpNameAlpha).set_attr_value(alpha_tensor);
        TensorDesc beta_desc(ge::Shape({1}), FORMAT_ND, DT_FLOAT);
        Tensor beta_tensor(beta_desc, reinterpret_cast<const uint8_t *>(&kValue1), sizeof(float));
        auto beta = op::Const(kOpNameBeta).set_attr_value(beta_tensor);

        auto gemm = op::GEMM(kOpNameGEMM);
        gemm.set_attr_transpose_a(transpose_a)
            .set_attr_transpose_b(transpose_b);
        gemm.update_input_desc_alpha(alpha_desc);
        gemm.update_input_desc_beta(beta_desc);

        // Add the alpha, beta, and gemm nodes to the graph.
        auto node_alpha = graph->AddNodeByOp(alpha);
        auto node_beta = graph->AddNodeByOp(beta);
        node_gemm = graph->AddNodeByOp(gemm);

        // Establish data edges between alpha, beta, and GEMM.
        auto ret = graph->AddDataEdge(node_alpha, kIndex0, node_gemm, kIndex3);
        ret = graph->AddDataEdge(node_beta, kIndex0, node_gemm, kIndex4);
    }
    // 4. Add the input and output of the new node
    bool AddInputsAndOutputs(GraphPtr &graph, const GNode &src_node, const GNode &dst_node, GNode &node_gemm) {
        auto [a, a_output_index] = src_node.GetInDataNodesAndPortIndexs(kIndex0);
        auto [b, b_output_index] = src_node.GetInDataNodesAndPortIndexs(kIndex1);
        int32_t add_node_c_input_index = -1;
        for (size_t i = 0; i < dst_node.GetInputsSize(); ++i) {
            auto [in_node, _] = dst_node.GetInDataNodesAndPortIndexs(i);
            AscendString node_name;
            auto ret = in_node->GetName(node_name);
            if (node_name != kOpNameMatMul) {
                add_node_c_input_index = i;
                break;
            }
        }
        if (add_node_c_input_index == -1) {
            return false;
        }
        auto [c, c_output_index] = dst_node.GetInDataNodesAndPortIndexs(add_node_c_input_index);
        // Establish the data edges between a, b, c and GEMM
        auto ret = graph->AddDataEdge(*a, a_output_index, node_gemm, kIndex0);
        if (ret != GRAPH_SUCCESS) {
            return false;
        }
        ret = graph->AddDataEdge(*b, b_output_index, node_gemm, kIndex1);
        ret = graph->AddDataEdge(*c, c_output_index, node_gemm, kIndex2);

        // Update the input/output description of GEMM
        TensorDesc input_desc_a;
        ret = src_node.GetInputDesc(kIndex0, input_desc_a);
        ret = node_gemm.UpdateInputDesc(kIndex0, input_desc_a);

        TensorDesc input_desc_b;
        ret = src_node.GetInputDesc(kIndex1, input_desc_b);
        ret = node_gemm.UpdateInputDesc(kIndex1, input_desc_b);

        TensorDesc input_desc_c;
        ret = dst_node.GetInputDesc(add_node_c_input_index, input_desc_c);
        ret = node_gemm.UpdateInputDesc(kIndex2, input_desc_c);

        TensorDesc output_desc_y;
        ret = dst_node.GetOutputDesc(kIndex0, output_desc_y);
        ret = node_gemm.UpdateOutputDesc(kIndex0, output_desc_y);
        return true;
    }
    // 5. Delete the old nodes and their edge relationships, and connect the new GEMM node to the output node
    void RemoveOldNodesEdgesAndAddGemmOutput(GraphPtr &graph, GNode &src_node, GNode &dst_node, GNode &node_gemm) {
        vector<GNode> node_vec{src_node, dst_node};
        // Delete all input edges of the old nodes
        for (auto &node: node_vec) {
            for (size_t i = 0; i < node.GetInputsSize(); ++i) {
                auto [in_node, in_id] = node.GetInDataNodesAndPortIndexs(i);
                if (in_node != nullptr) {
                    auto ret = graph->RemoveEdge(*in_node, in_id, node, i);
                }
            }
        }

        // Change the output edges of the old nodes to connect to the new GEMM
        for (auto &[out_node, out_id]: dst_node.GetOutDataNodesAndPortIndexs(kIndex0)) {
            if (out_node != nullptr) {
                auto ret = graph->RemoveEdge(dst_node, kIndex0, *out_node, out_id);
                ret = graph->AddDataEdge(node_gemm, kIndex0, *out_node, out_id);
            }
        }
        // Delete the old nodes at last
        for (auto &node: node_vec) {
            auto ret = graph->RemoveNode(node);
        }
    }
    } // namespace

    // |o>-----------------------------------
    // |o>    a  b
    // |o>    \ /              a   b    c
    // |o>   MatMul  c   ==>   \   |   /
    // |o>     \    /            GEMM
    // |o>      Add
    // |o>-----------------------------------
    // Fusion description: This example identifies the MatMul+Add structure on the left of the diagram above and replaces it with the single GEMM node on the right through the graph modification API
    // Return value description of the graph modification API: The graph modification APIs in this file require return value checking. For readability, except for the Pass entry function, the graph modification APIs in other functions only receive the return value without adding return value handling code. If return value checking is required, use the custom_context.SetErrorMessage("xxx") method together
    ```

3. Register the custom graph modification Pass.

    ```c++
    graphStatus FuseMatMulAndAddPass(GraphPtr &graph, CustomPassContext &custom_context) {
        cout << "FuseMatMulAndAddPass begin." << endl;
        GNode src_node;
        GNode dst_node;
        // 1. Traverse all nodes to find MatMul and Add nodes.
        if (!FindNodes(graph, src_node, dst_node)) {
            cout << "Do not find MatMul or Add node." << endl;
            return GRAPH_SUCCESS;
        }

        // 2. Check whether MatMul and Add nodes have an edge relationship.
        if (!CheckNodesHaveEdge(graph, src_node, dst_node)) {
            cout << "There is no edge between src and dst node." << endl;
            return GRAPH_SUCCESS;
        }

        // 3. Create and add a GEMM node.
        GNode node_gemm;
        CreateGEMMNode(graph, src_node, node_gemm);

        // 4. Add the inputs and outputs of the new node.
        if (!AddInputsAndOutputs(graph, src_node, dst_node, node_gemm)) {
            custom_context.SetErrorMessage("Add inputs and outputs failed.");
            return -1;
        }

        // 5. Delete the old nodes and their edge relationships, and connect the new GEMM node to the output node.
        RemoveOldNodesEdgesAndAddGemmOutput(graph, src_node, dst_node, node_gemm);

        cout << "FuseMatMulAndAddPass end." << endl;
        return GRAPH_SUCCESS;
    }
    // Use the REGISTER_CUSTOM_PASS registration macro to register the graph modification pass and specify the phase to be invoked.
    REGISTER_CUSTOM_PASS("FuseMatMulAndAddPass").CustomPassFn(FuseMatMulAndAddPass).Stage(CustomPassStage::kBeforeInferShape);
    ```

## How to Use a Custom Pass

After the custom pass is implemented as described above, this section briefly describes how to compile the graph modification function into a dynamic library plugin so that the registered pass is invoked by the framework at the very beginning of graph compilation. For detailed usage instructions, see [Sample Usage Guide](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/3_ir/2_fuse_matmul_add_pass).

1. Compile the graph modification function in [Development Example](#development-example) into a dynamic library file ending with ".so".
2. Copy the ".so" dynamic library file to the `${INSTALL_DIR}/opp/vendors/xxx/custom_fusion_passes/` directory. (A symbolic link is also supported. The ".so" file must be readable by the executing user.)

    Multiple `${INSTALL_DIR}/opp/vendors/xxx` directories are sorted in text order and then traversed to search for the "custom\_fusion\_passes/" subdirectory. The ".so" files in a single subdirectory are loaded in text order, and files not ending with ".so" are skipped during loading.

    - $\{INSTALL\_DIR\} must be replaced with the file storage path after CANN software installation. For example, if CANN is installed by the root user, the default storage path is /usr/local/Ascend/cann.
    - _xxx_: There is exactly one level of custom directory.
    - custom\_fusion\_passes: This directory must not contain subdirectories.

3. The following entry points, among others, are supported for compiling model files:

    To check whether the custom pass takes effect, dump the graph before compiling the model: set the [DUMP\_GE\_GRAPH](../../../user_guides/env_vars/DUMP_GE_GRAPH.md) environment variable before execution, and then compile the model using one of the following entry points:

    - Use the ATC tool for model conversion. For details about how to use the ATC tool, see *[Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).
    - [Building a Graph into an Offline Model](../compile_and_run_graph/compile_graph_to_offline_model.md).
    - [Building and Running a Graph](../compile_and_run_graph/compile_and_run_graph.md).

## Result Verification

See [Sample Usage Guide ](https://gitee.com/ascend/samples/tree/master/cplusplus/level1_single_api/3_ir/2_fuse_matmul_add_pass)\> Program Execution \> Check the execution result.

After the dump environment variable is set and the program finishes execution, graph files such as ge\_onnx\*.pbtxt are generated in the current path. You can obtain the following two graphs and then view them using visualization software such as Netron:

- Specify the Pass execution phase before InferShape:
  - ge\_onnx\_xxxx\_PreRunBegin.pbtxt: graph before fusion
  - ge\_onnx\_xxxx\_RunCustomPassBeforeInfershape.pbtxt: graph after fusion

    Taking the custom Pass that fuses MatMul+Add into GEMM as an example, the graph structure before fusion is as follows:

    ![Illustration](../figures/fusion_pass_image1.png)

    The graph structure after modification by the custom Pass is shown below. It can be seen that the MatMul+Add structure has been replaced with a single GEMM node.

    ![Illustration](../figures/fusion_pass_image2.png)

- Specify that the Pass execution phase is after InferShape:

  - ge\_onnx\_xxxx\_PrepareAfterInferFormatAndShape.pbtxt: Graph before fusion
  - ge\_onnx\_xxxx\_RunCustomPass\_AfterInferShape.pbtxt: Graph after fusion

  Taking the custom Pass that fuses BatchMatMulV2 into Transpose+Mul+ReduceSum as an example, the graph structure before fusion is as follows:

  ![Illustration](../figures/fusion_pass_image3.png)

  The graph structure after modification by the custom Pass is as follows. It can be seen that BatchMatMulV2 has been replaced by three nodes: Transpose, Mul, and ReduceSum.

  ![Illustration](../figures/fusion_pass_image4.png)

- Specify that the Pass execution phase is after the built-in original graph fusion Pass:

  - ge\_onnx\_xxxx\_OptimizeOriginalGraph\_FeGraphFusionAfter.pbtxt: Graph before fusion
  - ge\_onnx\_xxxx\_RunCustomPassAfterBuiltinFusionPass.pbtxt: Graph after fusion

  Taking the insertion of the Abs operator in the middle of the Data-Sqrt structure of the subgraph as an example, the graph structure before fusion is as follows:

  ![Illustration](../figures/fusion_pass_image5.png)

  The graph structure after modification by the custom Pass is shown below. It can be seen that the Abs operator has been inserted into the Data-Sqrt structure.

  ![Illustration](../figures/fusion_pass_image6.png)
