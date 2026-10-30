# Modifying a Graph Using the Graph Modification API

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-21T09:26:10.924Z pushedAt=2026-10-28T03:30:34.481Z nl=2 -->

If you want to directly optimize the structure of a graph, for example, replace certain specific subgraphs with a fusion operator to reduce computation steps, external memory access, and scheduling time, or add an operator between certain operators, you can use the content in this section to directly modify the graph into the desired structure.

## Feature Overview

This section uses the example of adding operator C between operator A and operator B to describe how to modify a graph. The main APIs involved are as follows:

![Feature overview](../figures/feature_intro_1.png)

In addition to modifying a graph through the graph modification APIs, this manual also provides a way to modify a graph by encapsulating the graph modification functions into a custom Pass. For details, see [Pass Based on the Graph Modification API](../custom_pass_development/implement_pass_using_graph_api.md).

## Development Example

1. Include the required header files.

    ```c++
    #include "graph.h"
    #include "ascend_string.h"
    #include "ge_ir_build.h"
    #include "gnode.h"
    ```

2. (Optional) Before modifying the graph, you can call [aclgrphDumpGraph](../../../api/graph_engine_api/cpp/ge/aclgrphDumpGraph.md) to dump the graph to the local device and view the graph information.

    Note that the aclgrphDumpGraph API must be called after the SetInputs and SetOutputs APIs, for example:

    ```c++
        string op_name = "tc_ge_openpass_0001";
        // Create a graph object.
        Graph graph(op_name);
        // Create a data operator instance.
        auto data = op::Data("data").set_attr_index(0);
        // Define the description of the data tensor.
        TensorDesc data_desc2(ge::Shape({3, 3, 3, 3}), FORMAT_NHWC, DT_FLOAT);
        data.update_input_desc_x(data_desc2);
        data.update_output_desc_y(data_desc2);
        // Create a MatrixInverse operator instance and set its input to the output of data.
        auto matrixinverse = op::MatrixInverse("MatrixInverse").set_input_x(data);
        // Create a Square operator instance and set its input to the output of MatrixInverse.
        auto square1 = op::Square("square1").set_input_x(matrixinverse);
        std::vector<Operator> inputs{data};
        std::vector<Operator> outputs{data,square1};
        // Call the API to set the input and output operators of the graph.
        graph.SetInputs(inputs).SetOutputs(outputs);
        std::map<std::string, std::string> init_options = {
             {ge::ir_option::SOC_VERSION,"xxx"}
        };
        // Initialize the model and apply for resources.
        auto ret = aclgrphBuildInitialize(init_options);
        EXPECT_EQ(ret, GRAPH_SUCCESS);
        std::cout << "BuildInitialize before infershape Success." << std::endl;
        size_t filesize =24;
        const char* file = "tc_ge_openpass_0001_dump";
        // Export the input graph to a file.
        ret = ge::aclgrphDumpGraph(graph,file,filesize);
        if(ret != GRAPH_SUCCESS) {
            std::cout<<"dump graph failed."<<std::endl;
        }
        // Compile and generate an offline model and save it to the memory buffer.
        ret = aclgrphBuildModel(graph,op_name);
        if(ret != GRAPH_SUCCESS) {
            std::cout<<"aclgrphBuildModel failed."<<std::endl;
        }
    ```

3. Add operator C between operator A and operator B, for example, insert Abs between the Const operator and the Add operator.

    ```c++
    const std::string CONST = "Const";
    const std::string ADD = "Add";
    GNode src_node;
    GNode dst_node;
    std::vector<GNode> nodes = graph.GetAllNodes();
    for(auto &node : nodes) {
      ge::AscendString name;
      node.GetName(name);
      std::string node_name(name.GetString());
      if(node_name == CONST) { src_node = node;}
      else if(node_name == ADD) { dst_node = node;}
    }
    graph.RemoveEdge(src_node, 0, dst_node, 0);
    auto abs = op::Abs("input3_abs");
    GNode node_abs = graph.AddNodeByOp(abs);
    TensorDesc output_tensor_desc;
    src_node.GetOutputDesc(0, output_tensor_desc);
    abs.UpdateInputDesc(0,  output_tensor_desc);
    abs.UpdateOutputDesc(0, output_tensor_desc);
    graph.AddDataEdge(src_node, 0, node_abs, 0);
    graph.AddDataEdge(node_abs, 0, dst_node, 0);
    ```

    1. Call [GetAllNodes](../../../api/graph_engine_api/cpp/ge/Graph/GetAllNodes.md) to find the Const operator and the Add operator.
    2. Call [RemoveEdge](../../../api/graph_engine_api/cpp/ge/Graph/RemoveEdge.md) to remove the edge (data edge or control edge) between the Const operator and the Add operator.
    3. Refer to [Defining Operators Using Operator Prototype Derived APIs](construct_graph_using_op_proto.md#defining-operators-using-operator-prototype-derived-apis) to create the Operator class operator Abs (you can also call OperatorFactory::CreateOperator to create the operator).
    4. Call [AddNodeByOp](../../../api/graph_engine_api/cpp/ge/Graph/AddNodeByOp.md) to create the GNode class operator Abs.

        After the operator is created, you can update the input and output TensorDesc of the operator as needed. Generally, update the Input TensorDesc and Output TensorDesc of the Abs operator based on the Output TensorDesc of the source node. If they are not updated, the system sets default values and derives the tensor shape and type during model compilation.

    5. Call [AddDataEdge](../../../api/graph_engine_api/cpp/ge/Graph/AddDataEdge.md) to add the edges between the Const operator and the Abs operator, and between the Abs operator and the Add operator. If there are control edges, call [AddControlEdge](../../../api/graph_engine_api/cpp/ge/Graph/AddControlEdge.md) to add the control edges.

    If multiple operators are inserted between A and B, for example, A-\>C-\>D-\>B, perform the operations A-\>C, C-\>D, D-\>B respectively by referring to the preceding steps.

4. Delete operator C between operator A and operator B, for example, delete Abs between Const and Add.

    ```c++
    graph.RemoveNode(node_abs);
    graph.AddDataEdge(src_node, 0, dst_node, 0);
    ```

    1. Call [RemoveNode](../../../api/graph_engine_api/cpp/ge/Graph/RemoveNode.md) to delete the Abs operator.
    2. Call [AddDataEdge](../../../api/graph_engine_api/cpp/ge/Graph/AddDataEdge.md) to add the edge between the Const and Add operators. If there is a control edge, call [AddControlEdge](../../../api/graph_engine_api/cpp/ge/Graph/AddControlEdge.md) to add the control edge.

5. In addition, if you need to query the information of a GNode, refer to the methods provided by [GNode](../../../api/graph_engine_api/cpp/ge/GNode/GNode.md).
