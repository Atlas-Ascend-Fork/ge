# Building a FlowGraph with FlowOperator

<!-- md-trans-meta sourceCommit=cc5ba9391389e272005d4ade3dda4ccce784361d translatedAt=2026-10-28T12:58:46.950Z pushedAt=2026-10-29T03:25:50.697Z nl=2 -->

## What Is FlowOperator

FlowOperator is the base class of nodes in a FlowGraph, with two derived types: FlowData and FlowNode.

### What Is FlowData

FlowData is the input node of a FlowGraph. FlowData defines the name and index of the input node. For details, see the FlowData class.

### What Is FlowNode

FlowNode is the compute node of a FlowGraph. FlowNode defines the name, number of inputs, and number of outputs of the compute node. The actual computation logic of FlowNode is determined by the ProcessPoint added through the AddPp method. A ProcessPoint can be either a FunctionPp or a GraphPp. For details, see the FlowNode class.

- GraphPp

  The main function prototypes of GraphPp are as follows.

  ```cpp
  GraphPp(const char *pp_name, const Graphbuilder &builder) // GraphPp constructor.
  GraphPp &SetCompileConfig(const char *json_file) // Set the path and file name of the JSON configuration file of GraphPp. Used for mapping between GraphPp and AscendGraph.
  ```

  Take the Add function as an example to describe how to build a GraphPp. In the following example, Add has two inputs, data0 and data1. The mapping between GraphPp and AscendGraph is passed through the BuildGraph parameter in the GraphPp constructor, and add\_graph\_config.json is used to configure the compilation information of AscendGraph.

  ```cpp
  ge::Graph BuildGraph()
  {
      auto data0 = op::Data("data0").set_attr_index(0);
      auto data1 = op::Data("data1").set_attr_index(1);
      auto add = op::Add("add").set_input_x1(data0).set_input_x2(data1);
      ge::Graph graph("Graph");
      graph.SetInputs({data0, data1}).SetOutputs({add});
      return graph;
  }
  auto graph_pp = dflow::GraphPp("graph_pp", BuildGraph)
                  .SetCompileConfig("add_graph_config.json");
  ```

- FunctionPp

  The main function prototypes of FunctionPp are as follows.

  ```cpp
  FunctionPp(const char_t *pp_name) // FunctionPp constructor.
  FunctionPp &SetCompileConfig(const char *json_file)  // Used for mapping between FunctionPp and UDF. The json_file configures the compilation information of the UDF.
  FunctionPp &SetInitParam(const char *attr_name, const ge::DataType &value) // Set the initialization parameters of FunctionPp.
  ```

  Take the Add function as an example to describe how to build a FunctionPp. In the following example, the name of the FunctionPp is "func\_pp", the function of the FunctionPp is implemented through UDF, and the mapping between FunctionPp and UDF is implemented through add\_func\_config.json.

  > [!NOTE]
  > The UDF implementation can use C++ or Python.

  ```cpp
  auto function_pp = dflow::FunctionPp("func_pp")
                      .SetCompileConfig("add_func_config.json")
                      .SetInitParam("out_type", ge::DT_UINT32);
  ```

## How to Build a FlowGraph Using FlowOperator

### Feature Overview

You can use the created FlowOperator nodes to create a FlowGraph instance, and set the inputs and outputs in the FlowGraph to complete graph construction.

![](figures/260109163332946.png)

### Defining Nodes Using FlowOperator-Derived APIs

1. Include the header files.

   ```cpp
   #include "flow_graph/data_flow.h"
   ```

2. Create a FlowOperator instance.

   The following two types are supported:

    - FlowData

        ```cpp
        auto data0 = dflow::FlowData("Data0", 0);
        auto data1 = dflow::FlowData("Data1", 1);
        ```

    - FlowNode

        ```cpp
        auto node0 = dflow::FlowNode("node0", 2, 1);
        ```

    Note: The data names and node names in the graph must be unique.

3. Set the FlowOperator input.

    Only FlowOperator instances of the FlowNode type require input settings.

    Set the FlowOperator input by calling "SetInput", for example:

    ```cpp
    auto node0 = dflow::FlowNode("node0", 2, 1)         // Create a FlowOperator instance of the FlowNode type
      .SetInput(0, data0)                               // Set the first input of FlowNode to data0
      .SetInput(1, data1);                              // Set the second input of FlowNode to data1
    ```

### Expressing FlowOperator Connection Edges

The edges between FlowOperators are data edges. A data edge specifies the input of a FlowOperator. Data edges are expressed as follows.

![](figures/260109163658562.png)

The sample code and comments in the preceding figure are as follows.

```cpp
auto data0 = dflow::FlowData("Data0", 0);
auto data1 = dflow::FlowData("Data1", 1);
auto node0 = dflow::FlowNode("flow_node0", 2, 2).SetInput(0, data0).SetInput(1, data1); // FlowData has only one output. Therefore, when it is used as the input of a FlowNode, the output index of FlowData does not need to be passed to SetInput.
auto node1 = dflow::FlowNode("flow_node1", 1, 1).SetInput(0, flow_node0, 0); // node0 has two outputs. The first output is used as the first input of node1. Therefore, the output index 0 of node0 needs to be passed to SetInput.
auto node2 = dflow::FlowNode("flow_node2", 1, 1).SetInput(0, flow_node0, 1); // node0 has two outputs. The second output is used as the first input of node2. Therefore, the output index 1 of node0 needs to be passed to SetInput.
```

### Creating a FlowGraph Instance

After the FlowOperators are ready, you need to create a FlowGraph instance and set the input FlowOperators and output FlowOperators in the FlowGraph. The main process is as follows:

1. Include the required header files.

    ```cpp
    #include "flow_graph/data_flow.h"
    ```

2. Create a FlowGraph object.

    ```cpp
    dflow::FlowGraph flow_graph("flow_graph");
    ```

    For details about the related APIs, see the FlowGraph class.

3. Set the input and output nodes of the FlowGraph. The main APIs used are as follows:

    - Set the input nodes in the FlowGraph: SetInputs.
    - Set the output nodes in the FlowGraph: SetOutputs.
    - If a FlowNode used as an output node has multiple outputs, you can also specify one of its outputs as the model output by using SetOutputs(index).

    The following example shows the scenario where all outputs of node2 are used as the model outputs. If you need to set only some outputs of a node as the model outputs, see SetOutputs(index):

    ```cpp
    std::vector<FlowOperator> inputs{data0, data1};
    std::vector<FlowOperator> outputs{node2};
    flow_graph.SetInputs(inputs).SetOutputs(outputs);
    ```

    If the input consists of multiple FlowData nodes, ensure that the order of the inputs parameter is consistent with the order specified by the index attribute of the FlowData nodes. Otherwise, an error will be reported when the model is generated later. For example:

    ```cpp
    // Prepare the first input data
    auto data0 = dflow::FlowData("Data0", 0);   // Create the data0 node with the index attribute set to 0
    // Prepare the second input data
    auto data1 = dflow::FlowData("Data1", 1);   // Create the data1 node with the index attribute set to 1
    // Set the FlowGraph input nodes
    std::vector<FlowOperator> inputs{data0, data1};
    ```
