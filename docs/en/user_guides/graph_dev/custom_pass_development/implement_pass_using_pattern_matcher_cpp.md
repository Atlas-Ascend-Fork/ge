# Implementing Pass Based on Pattern Mapping (C++)

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-21T09:48:16.640Z pushedAt=2026-10-28T07:20:42.540Z nl=2 -->

To improve the development efficiency of custom fusion passes, this chapter provides a set of graph-structure-based matching and replacement APIs for building passes. The logic of the entire fusion behavior can be divided into the following three steps:

1. Matching: Define a graph structure through a Pattern to match and search for subgraph structures.
2. Decision-making: Based on the matching result and more specific conditions, determine whether the match meets the fusion requirements.
3. Replacement: After confirming that fusion is possible, perform the graph structure replacement to complete the optimization.

**Figure 1** Logical architecture diagram<a id="fig1"></a>
![Illustration](../figures/logical_architecture.png "Logical architecture diagram")

The logical architecture is shown in the preceding figure. The related core concepts are described as follows:

- Pattern: In the graph matching process, a Pattern is a template or rule that describes a subgraph structure. It is used to search for subgraphs that meet specific rules in a Graph through a graph matching algorithm.
- PatternMatcher: The core object that executes the matching algorithm. It receives a Pattern and searches the Graph for subgraphs that conform to the subgraph structure defined in the Pattern.
- GraphRewriter: The core object that executes graph rewriting. It receives the matched subgraph boundaries and the target Replacement graph (after replacement), and replaces the original subgraph nodes with the node structure in the Replacement to complete the graph reconstruction.

Developers implement a custom fusion pass by inheriting the base classes provided by GE and overriding their methods, and then call the registration macro to register the pass to the specified phase. Depending on the application scenario, GE provides two types of base classes for inheritance:

- General subgraph fusion (1:1 or complex topology replacement) scenario: applicable to scenarios where a complete subgraph structure needs to be matched and replaced as a whole with another subgraph.

    Inherit the [PatternFusionPass](../../../api/graph_engine_api/cpp/ge/fusion/PatternFusionPass/PatternFusionPass.md) class to implement a custom fusion pass class, and register the pass to the specified phase through the [REG\_FUSION\_PASS](../../../api/graph_engine_api/cpp/ge/fusion/REG_FUSION_PASS.md) registration macro.

- Single-node replacement (replacing a single node with N nodes) scenario: inherit the [DecomposePass](../../../api/graph_engine_api/cpp/ge/DecomposePass/DecomposePass.md) class to implement a custom fusion pass class, and register the pass to the specified phase through the [REG\_DECOMPOSE\_PASS](../../../api/graph_engine_api/cpp/ge/REG_DECOMPOSE_PASS.md) registration macro.

The two scenarios are similar in usage, but differ in the granularity of the matched object (subgraph/single node) and the replacement method (complex topology replacement/multi-node expansion). They are described in detail below.

## Scenario Introduction

### General Subgraph Fusion (1:1 or Complex Topology Replacement) Scenario Fusion Pass Development

This section first introduces PatternFusionPass, the core data structure involved in this scenario, then describes the three functions that need to be overridden during development: Patterns, MeetRequirements, and Replacement, and finally explains how to register the Pass to the specified phase.

The **PatternFusionPass** class is declared as follows:

```c++
class PatternFusionPass : public FusionBasePass {
  public:
  Status Run(GraphPtr &graph, CustomPassContext &pass_context) override;
     protected:
      virtual std::vector<PatternUniqPtr> Patterns() = 0;
      virtual bool MeetRequirements(const std::unique_ptr<MatchResult> &match_result);
      virtual GraphUniqPtr Replacement(const std::unique_ptr<MatchResult> &match_result) = 0;
    };
```

The Run function calls Patterns to obtain the topological Pattern of the template, matches the Pattern one by one in the target Graph, calls MeetRequirements to determine whether each matched Pattern needs to be replaced, and finally obtains the target structure through Replacement to replace the Patterns that meet the replacement conditions.

Implement custom fusion Pass development by inheriting the PatternFusionPass class and overriding the Patterns, MeetRequirements, and Replacement functions. The functions are described as follows:

|Function|Description|Whether It Must Be Overridden|
|--|--|--|
|Patterns|Defines the template topology to be matched in the target graph; returns one or more graph structure pointers.|Yes|
|MeetRequirements|Filters the graph structures matched by Patterns based on conditions; takes the match result as input and returns a Boolean value.|No. By default, it directly returns true.|
|Replacement|Defines the replacement structure; takes the match result as input and returns a graph pointer.|Yes|

#### Patterns

Patterns is used to define one or more template topologies to be matched in the target graph. It uses [EsGraphBuilder](../../../api/graph_engine_api/cpp/ge/es/EsGraphBuilder/EsGraphBuilder.md) to build a DAG (Directed Acyclic Graph) to express the pattern, as follows:

```c++
std::vector<PatternUniqPtr> Patterns() override {
  std::vector<PatternUniqPtr> patterns;
  // Use EsGraphBuilder to build the pattern
  auto graph_builder = es::EsGraphBuilder("pattern");
  // Define the pattern here
  // ...
  // Initialize the Pattern object. Replace xxx with the actual output node
  auto graph = graph_builder.BuildAndReset({xxx});
  auto pattern = std::make_unique<Pattern>(std::move(*graph));
  patterns.emplace_back(std::move(pattern));
  // You can continue adding multiple patterns to patterns
  // ...
  return patterns;
}

```

EsGraphBuilder is a graph builder class used to build computation graphs. It is recommended that developers use the [ES Interface](../../../api/graph_engine_api/cpp/ge/es/es_interface.md) to define patterns. The ES API provides interfaces for defining inputs, constants, and operators. The following is an example of using the ES API to define a ReLu single-operator pattern:

```c++
std::vector<PatternUniqPtr> patterns;
// Create an EsGraphBuilder instance to build a computation graph named pattern
auto graph_builder = es::EsGraphBuilder("pattern");
auto data = graph_builder.CreateInput(0);
auto relu = es::Relu(data);
// Build and reset the graph, using {relu} as the output node
auto graph = graph_builder.BuildAndReset({relu});
// Move the graph into the Pattern constructor to create a pattern object
auto pattern = std::make_unique<Pattern>(std::move(*graph));
patterns.emplace_back(std::move(pattern));

```

> [!NOTE]
>
> The Pattern used for matching must be self-contained (except for the boundary output operators, the data output consumers of all operators within the boundary must also be within the boundary). A non-self-contained Pattern will not be matched.

In addition to using EsGraphBuilder to build a Pattern as described above, GE provides two other APIs for defining a Pattern at a finer granularity:

- [CaptureTensor](../../../api/graph_engine_api/cpp/ge/fusion/Pattern/CaptureTensor.md)

  During definition, a Tensor in the Pattern can be captured so that it can be obtained in order from [MatchResult](../../../api/graph_engine_api/cpp/ge/fusion/MatchResult/MatchResult.md). The method declaration is as follows. The input parameter node\_output is of the NodeIo type, which consists of a node and an index, representing a specific output of a node.

  ```c++
  // CaptureTensor declaration
  Pattern &CaptureTensor(const NodeIo &node_output);

  // NodeIo struct
  struct NodeIo {
   GNode node;
    int64_t index;
  };

  ```

  The following example shows how to call CaptureTensor to capture data:

  ```c++
  std::vector<PatternUniqPtr> patterns;
  // Create an EsGraphBuilder instance to build a compute graph named pattern
  auto graph_builder = es::EsGraphBuilder("pattern");
  auto data = graph_builder.CreateInput(0);
  auto relu = es::Relu(data);
  // Build the compute graph, which contains only the data -> ReLU(relu) structure
  auto graph = graph_builder.BuildAndReset({relu});
  // Create a Pattern instance and initialize it with the built graph
  auto pattern = std::make_unique<Pattern>(std::move(*graph));
  // Call CaptureTensor to capture data.
  pattern->CaptureTensor({*relu.GetProducer(), 0});
  patterns.emplace_back(std::move(pattern));
  ```

- [PatternMatcherConfig](../../../api/graph_engine_api/cpp/ge/fusion/PatternMatcherConfig/PatternMatcherConfig.md)

     When constructing a custom Pass, you can pass in PatternMatcherConfig to enable Const value matching and the matching of IR attributes and their values. The constructor of the base class PatternFusionPass is as follows:

    ```c++
    explicit PatternFusionPass(std::unique_ptr<PatternMatcherConfig> match_config);
    ```

    Use [PatternMatcherConfigBuilder](../../../api/graph_engine_api/cpp/ge/fusion/PatternMatcherConfigBuilder/PatternMatcherConfigBuilder.md) to construct PatternMatcherConfig. The PatternMatcherConfigBuilder class provides two functions as switches for matching capabilities:

  - EnableConstValueMatch: Enables Const value matching. During matching, the values of Const/Constant defined in the Pattern are matched, and the match is considered successful only when the values are equal.
  - EnableIrAttrMatch: Enables matching of IR attributes and their values. During Pattern matching, the Pass matches the number and values of IR attributes carried on the nodes in the Pattern.

  The following is an example of a constructor that enables Const value matching for a custom Pass class named CustomFusionPass:

  ```c++
  explicit CustomFusionPass() : PatternFusionPass(PatternMatcherConfigBuilder().EnableConstValueMatch().Build()) {}
  ```

#### MeetRequirements

For the match results obtained by Patterns, filter them in MeetRequirements. As shown in the implementation of the Run function above, each match result of the [MatchResult](../../../api/graph_engine_api/cpp/ge/fusion/MatchResult/MatchResult.md) type is passed to MeetRequirements as an input parameter. Through MatchResult, developers can obtain the information of the match result for filtering, and the returned Boolean value determines whether to replace the match result, as follows:

```c++
bool MeetRequirements(const std::unique_ptr<MatchResult> &match_result) override {
  // Filter the match result using the passed-in match_result
  // Return true if the condition is met
  if (IsSatisfy(match_result)) {
    return true;
  }
  // Return false if the condition is not met
  return false;
}
```

MatchResult is the match result class, which contains information such as the nodes and edges of the match result. Developers can use the MatchResult member functions to obtain the relevant information of the match result for filtering. The following is an example of using the [GetCapturedTensor](../../../api/graph_engine_api/cpp/ge/fusion/MatchResult/GetCapturedTensor.md) member function to verify whether the ReLu output is a dynamic shape:

```c++
NodeIo relu_output;
// Try to obtain the first captured output tensor from match_result and store it in relu_output
if(match_result->GetCapturedTensor(0,relu_output) != GRAPH_SUCCESS){
  return false;
}
TensorDesc relu_out_tensor_desc;
// Obtain the output tensor description from relu_output
relu_output.node.GetOutputDesc(relu_output.index, relu_out_tensor_desc);
if (relu_out_tensor_desc.GetShape().GetShapeSize() != -1){
  return false;
}
return true;
```

#### Replacement

Replacement defines the target structure, which replaces the part that matches Patterns and for which MeetRequirements returns true. As with Patterns, the structure is defined using EsGraphBuilder, so it is not described again here:

```c++
GraphUniqPtr Replacement(const std::unique_ptr<MatchResult> &match_result) override {
  auto replacement_graph_builder = es::EsGraphBuilder("replacement");
  // Define the replacement structure here
  // ...
  return replacement_graph_builder.BuildAndReset({r_a});
}
```

> [!NOTE]
>
> If the Pass registration phase is after InferShape, you need to call GeUtils::[InferShape](../../../api/graph_engine_api/cpp/ge/GeUtils/InferShape.md) in Replacement. In addition, if you want to use GeUtils::[CheckNodeSupportOnAicore](../../../api/graph_engine_api/cpp/ge/GeUtils/CheckNodeSupportOnAicore.md) to determine whether the target structure is supported, this function must be called after InferShape.

**Register the custom fusion Pass**:

After the fusion Pass is defined, use the registration macro [REG\_FUSION\_PASS](../../../api/graph_engine_api/cpp/ge/fusion/REG_FUSION_PASS.md) to register it to the corresponding phase. The following is an example of registering a custom Pass named CustomFusionPass to the kBeforeInferShape phase:

```c++
REG_FUSION_PASS(CustomFusionPass).Stage(CustomPassStage::kBeforeInferShape);
```

For details about each phase, see [Stage](../../../api/graph_engine_api/cpp/ge/fusion/FusionPassRegistrationData/Stage.md).

### Single-Node Replacement (Replacing a Single Node with N Nodes) Scenario Fusion Pass Development

In this scenario, the base class inherited by the Pass is DecomposePass. Because the structure to be replaced is a single node, the Pattern no longer needs to be defined through Patterns here. Instead, the operator type is directly passed in the constructor, as follows:

```c++
class CustomOne2NPass : public DecomposePass {
 public:
  CustomOne2NPass(const std::vector<AscendString> &op_types) : DecomposePass(op_types) {}
};
```

Similar to the general scenario, a Pass inherited from DecomposePass also needs to override MeetRequirements and Replacement. However, the input parameter type of these two methods is no longer MatchResult but [GNode](../../../api/graph_engine_api/cpp/ge/GNode/GNode.md), that is, the node matched in the graph by the op\_types passed during construction.

```c++
bool MeetRequirements(const GNode &matched_node) override {
    ...
}
GraphUniqPtr Replacement(const GNode &matched_node) override {
    ...
}
```

**Register a custom fusion Pass**:

The following is an example of using the registration macro [REG\_DECOMPOSE\_PASS](../../../api/graph_engine_api/cpp/ge/REG_DECOMPOSE_PASS.md) to initialize CustomOne2NPass with Conv2D as the op\_types and register it to the kAfterInferShape phase:

```c++
REG_DECOMPOSE_PASS(CustomOne2NPass, {"Conv2D"}).Stage(CustomPassStage::kAfterInferShape);
```

## Development Example

This section uses the fusion of the MatMul+Add structure into a GEMM custom pass as an example (corresponding to the fusion pass development in the general scenario described above) to detail how to modify a graph through a custom fusion pass. For details, see [Sample Source Code](../../../../../examples/fusion_pass/pattern_base_pass/1_fuse_matmul_add_pass/cpp/README.md). The sample repository also provides more samples. You can click [Fusion Pass Samples](../../../../../examples/fusion_pass/pattern_base_pass/README.md) to view them.

The graph structures before and after modification are as follows. In this example, the MatMul+Add structure on the left of the figure is identified and replaced with a single GEMM node on the right through the graph modification API:

```text
// |o>-----------------------------------
// |o>      a  b
// |o>      \ /                a    b    c
// |o>     MatMul     c   ==>   \   |   /
// |o>        \      /            GEMM
// |o>           Add
// |o>-----------------------------------
```

1. Include the header files.

    ```c++
    #include <iostream>
    // Header file of the custom fusion pass API
    #include "ge/fusion/pass/pattern_fusion_pass.h"
    // Header file of the ES API
    #include "es_all_ops.h"
    ```

2. Use the custom pass to modify the graph.

    ```c++
    class FuseMatMulAndAddPass : public PatternFusionPass {
     protected:
      // Override Patterns
      std::vector<PatternUniqPtr> Patterns() override {
        std::cout << "Define pattern for FuseMatMulAndAddPass" << std::endl;
        std::vector<PatternUniqPtr> patterns;
        // Create an EsGraphBuilder instance to build the compute graph. The graph is named pattern0.
        auto graph_builder0 = es::EsGraphBuilder("pattern0");
        auto [a0, b0, c0] = graph_builder0.CreateInputs<3>();
        auto matmul0 = es::MatMul(a0, b0);
        auto add0 = es::Add(matmul0, c0);
        // Build and reset the graph
        auto graph0 = graph_builder0.BuildAndReset({add0});
        auto pattern0 = std::make_unique<Pattern>(std::move(*graph0));
        patterns.emplace_back(std::move(pattern0));

        return patterns;
      }
      // Override Replacement
      GraphUniqPtr Replacement(const std::unique_ptr<MatchResult> &match_result) override {
        std::cout << "Define replacement for FuseMatMulAndAddPass" << std::endl;
        // Build the replacement graph
        auto replace_graph_builder = es::EsGraphBuilder("replacement");
        auto [r_a, r_b, r_c] = replace_graph_builder.CreateInputs<3>();
        auto alpha_const = replace_graph_builder.CreateScalar(1);
        auto beta_const = replace_graph_builder.CreateScalar(1);
        auto gemm = es::GEMM(r_a, r_b, r_c, alpha_const, beta_const);
        // Build and reset the graph
        return replace_graph_builder.BuildAndReset({gemm});
      }
    };
    ```

3. Register the custom fusion Pass.

    ```c++
    // Use the REG_FUSION_PASS registration macro to register the graph-modification Pass and specify the phase to be invoked
    REG_FUSION_PASS(FuseMatMulAndAddPass).Stage(CustomPassStage::kBeforeInferShape);
    ```

## How to Use a Custom Pass

After completing the custom pass described above, this section briefly describes how to compile the graph modification function into a dynamic library plugin so that the registered pass is invoked by the framework during the graph compilation phase. For detailed usage instructions, see [Sample Usage Guide](../../../../../examples/fusion_pass/pattern_base_pass/1_fuse_matmul_add_pass/cpp/README.md).

1. Compile the graph modification function in [Development Example](#development-example) into a dynamic library file ending with ".so".
2. After successful compilation, run the **make install** command to install the ".so" dynamic library file to the `${INSTALL_DIR}/opp/vendors/xxx/custom_fusion_passes/` directory. (Soft links are supported. The ".so" file must be readable by the executing user.)

    Multiple `${INSTALL_DIR}/opp/vendors/xxx` directories are sorted alphabetically and then traversed to search for the "custom\_fusion\_passes/" subdirectory. The ".so" files within a single subdirectory are loaded in alphabetical order, and files not ending with ".so" are skipped during loading.

    - Here, $\{INSTALL\_DIR\} must be replaced with the file storage path after the CANN software is installed. Taking installation by the root user as an example, the default storage path after installation is /usr/local/Ascend/cann.
    - _xxx_: There is exactly one level of custom directory.
    - custom\_fusion\_passes: This directory must not contain subdirectories.

3. The following entry points, among others, are supported for compiling model files:

    To check whether the custom pass described above takes effect, dump the graph before compiling the model: set the [DUMP\_GE\_GRAPH](../../../user_guides/env_vars/DUMP_GE_GRAPH.md) environment variable before execution, and then compile the model using one of the following entry points:

    - Use the ATC tool for model conversion. For details about how to use the ATC tool, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).
    - [Building a Graph into an Offline Model](../compile_and_run_graph/compile_graph_to_offline_model.md).
    - [Building and Running a Graph](../compile_and_run_graph/compile_and_run_graph.md).

## Result Verification

See [Sample Usage Guide](../../../../../examples/fusion_pass/pattern_base_pass/1_fuse_matmul_add_pass/cpp/README.md)\> Program Running \> to view the running result.

After the dump environment variable is set, when the program finishes execution, graph files such as ge\_onnx\*.pbtxt are generated in the current path. You can obtain the following two graphs (taking the example where the specified Pass execution phase is before InferShape), and then use visualization software such as Netron to view them:

- ge\_onnx\_xxxx\_PreRunBegin.pbtxt: graph before fusion
- ge\_onnx\_xxxx\_RunCustomPassBeforeInfershape.pbtxt: graph after fusion

The graph structure before fusion is as follows:

![](../figures/fusion_pass_image9.png)

The graph structure modified by the custom Pass is as follows. It can be seen that the MatMul+Add structure has been replaced by a single GEMM node.

![](../figures/fusion_pass_image10.png)
