# Implementing Pass Based on Pattern Mapping (Python)

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-21T09:49:07.286Z pushedAt=2026-10-28T07:20:48.899Z nl=2 -->

This document describes how to develop a custom fusion pass using Python. Python fusion passes and C++ fusion passes use the same GE matching and replacement mechanism, but Python is more suitable for rapid development and runtime integration, significantly reducing development costs. The main differences from the C++ implementation are as follows. The two implementations are completely consistent in their core mechanisms, both following the three-step process of "match-decision-replace", but the Python implementation has significant advantages in the following aspects:

| Comparison Dimension | Python Implementation | C++ Implementation |
| --- | --- | --- |
| Development mode | Directly loads .py files without compilation | Requires compilation into a .so dynamic library |
| Pattern definition | Supports the @pattern decorator and can use expressions for direct description | Uses EsGraphBuilder to build the pattern graph |
| Replacement definition | Simple replacement can be returned directly, with the expression serving as the replacement | Requires manual creation of the replacement graph |
| Iteration speed | Modify the file and re-trigger execution or reloading for verification | Requires recompilation and reinstallation after modification |
| Integration method | Configure the ASCEND_GE_PY_PASS_PATH environment variable | Place the compiled file in the specified directory |
| Applicable Scenarios | Rapid prototyping, rule verification, and iterative debugging | Production environments with high performance requirements |

The logic of the entire fusion behavior is likewise divided into three steps:

1. **Matching**: Define a graph structure through a Pattern to search for and match subgraph structures.
2. **Decision**: Based on the match result and more specific conditions, determine whether the match meets the fusion requirements.
3. **Replacement**: After confirming that fusion is possible, perform the graph structure replacement operation to complete the optimization.

The logical architecture is shown in [Figure 1](./implement_pass_using_pattern_matcher_cpp.md#fig1). Developers implement a custom fusion pass by inheriting the base classes provided by GE and overriding their methods. Depending on the application scenario, GE provides two types of base classes for inheritance in Python:

- **General subgraph fusion (1:1 or complex topology replacement) scenario**: Suitable for scenarios where a complete subgraph structure needs to be matched and replaced as a whole with another subgraph.

    Inherit the [PatternFusionPass](../../../api/graph_engine_api/python/ge/passes/PatternFusionPass/overview.md) class to implement a custom fusion pass class, and register the pass to the specified phase through the @[register\_fusion\_pass](../../../api/graph_engine_api/python/ge/passes/register_fusion_pass.md) decorator.

- **Single-node replacement (replacing a single node with N nodes) scenario**: Inherit the [DecomposePass](../../../api/graph_engine_api/python/ge/passes/DecomposePass/overview.md) class to implement a custom fusion Pass class, and register the Pass to the specified phase through the @[register\_decompose\_pass](../../../api/graph_engine_api/python/ge/passes/register_decompose_pass.md) decorator.

The two scenarios are similar in usage, but differ in the granularity of the matched object (subgraph/single node) and the replacement method (complex topology replacement/multi-node expansion). They are described in detail below.

## Scenarios

- **Fusion pass development for the general subgraph fusion (1:1 or complex topology replacement) scenario**

    This section first introduces the core data structure PatternFusionPass involved in this scenario, then describes the core methods that need to be overridden during development, and finally explains how to register the pass to a specified phase. The **PatternFusionPass** class is declared as follows:

    ```python
    class PatternFusionPass:
        def patterns(self) -> List[Pattern]:
            ...

        def meet_requirements(self, match_result: MatchResult) -> bool:
            return True

        def replacement(self, inputs, match_result: MatchResult = None):
            ...
    ```

    Developers implement custom fusion pass development by inheriting the PatternFusionPass class and overriding its core methods. The functions are described as follows:

    | Method | Description | Whether Override Is Required |
    | --- | --- | --- |
    | [patterns](../../../api/graph_engine_api/python/ge/passes/PatternFusionPass/patterns.md) | Defines what to match:<br>  - Method 1: a method decorated with @pattern: defines the template topology to be matched in the target graph; uses a Python expression to describe the pattern. The @pattern decorator is suitable for most common topologies. It automatically captures the visited external inputs and the returned pattern outputs, but does not automatically capture intermediate tensors that are not returned as outputs.<br>  - Method 2: manually builds the pattern graph and returns a pattern list. If meet_requirements or replacement needs to read intermediate tensors that are not returned as outputs, use the manual pattern construction method and call the capture_tensor method to mark the intermediate tensors that need to be read.<br>For rules such as Add(x, 0) and MatMul+Add, use @pattern first, because the code is shorter and closer to the optimization logic. | Yes, choose one of the two methods |
    | [meet_requirements](../../../api/graph_engine_api/python/ge/passes/PatternFusionPass/meet_requirements.md) | Determines whether replacement is possible: filters the graph structures matched by the pattern based on conditions; takes the match result as input and returns a Boolean value. | No, returns True by default |
    | [replacement](../../../api/graph_engine_api/python/ge/passes/PatternFusionPass/replacement.md) | Defines what to replace with: defines the replacement structure; takes the inputs object and match_result as input and returns the replaced structure. | Yes |

  - **Patterns**
    - **Using the @pattern decorator (recommended)**

        Using the @pattern decorator greatly simplifies pattern definition by directly describing the graph structure with a Python expression:

        ```python
        @register_fusion_pass(name="MyPass", stage=PassStage.BEFORE_INFER_SHAPE)
        class MyPass(PatternFusionPass):
            @pattern
            def my_pattern(self, inputs):
                return inputs[0] + inputs[1]  # The expression is the pattern

            def meet_requirements(self, match_result):
                # Conditional filtering (optional)
                return True

            def replacement(self, inputs):
                return inputs[0]  # The expression is the replacement
        ```

    - **Manually build a Pattern (consistent with C++)**

        When finer-grained control over the Pattern definition is required, you can manually build the Pattern graph in the same way as in C++:

        ```python
        from ge.es.graph_builder import GraphBuilder
        from ge.passes import create_pattern, create_replacement


        def patterns(self):
            builder = GraphBuilder("pattern")
            a, b, c = builder.create_inputs(3)
            matmul = MatMul(a, b)
            add = matmul + c
            pat = create_pattern(builder.build_and_reset([add]))
            pat.capture_tensor(matmul)
            pat.capture_tensor(add)
            return [pat]


        def replacement(self, match_result):
            builder = GraphBuilder("replacement")
            a, b, c = builder.create_inputs(3)
            gemm = GEMM(a, b, c, builder.create_scalar_float(1.0), builder.create_scalar_float(1.0))
            return create_replacement(builder.build_and_reset([gemm]))
        ```

  - **meet\_requirements**

    For the match results obtained by Patterns, filtering is performed in meet\_requirements. Each match result of the match\_result type is used as the input parameter of meet\_requirements. Through match\_result, developers can filter the information of the match result, and the returned Boolean value serves as the basis for whether to replace the match result.

    ```python
    def meet_requirements(self, match_result):
        # Traverse all nodes in the match result
        for node in match_result.get_matched_nodes():
            # If the current node is a constant node
            if node.type == "Const":
                return _is_zero(node.get_attr("value"))
        return False
    ```

    match\_result is the match result class, which contains information such as the nodes and edges of the match result. Developers can use the match\_result member function to obtain the relevant information of the match result for filtering:

    | Method | Description |
    | --- | --- |
    | [get_matched_nodes](../../../api/graph_engine_api/python/ge/passes/MatchResult/get_matched_nodes.md)() | Obtains the list of all matched nodes. |
    | [get_captured_tensor](../../../api/graph_engine_api/python/ge/passes/MatchResult/get_captured_tensor.md)(index, node_io) | Obtains the captured tensor at the specified index. |

    The following is an example of using the get\_captured\_tensor member function to verify whether the ReLU output is a dynamic shape:

    ```python
    def meet_requirements(self, match_result):
        from ge.graph.types import TensorDesc

        # Try to obtain the first captured output tensor from match_result.
        try:
            relu_output = match_result.get_captured_tensor(0)
            # Obtain the description information of the output tensor.
            relu_out_tensor_desc = relu_output.node.get_output_desc(relu_output.index)
            # Check whether it is a dynamic shape.
            if relu_out_tensor_desc.shape.size != -1:
                return False
            return True
        except Exception:
            return False
        ```

  - **replacement**

    The target structure is defined in replacement to replace the parts that match Patterns and for which `meet_requirements` returns True. Python provides two ways to define replacement:

    - **Method 1: Directly return an expression (simple scenario)**

        For simple replacement scenarios, you can directly return an expression, and GE automatically builds the replacement graph:

        ```python
        def replacement(self, inputs):
            # Directly return the 0th input, which is equivalent to deleting a node.
            return inputs[0]
        ```

        Alternatively, use an operator expression:

        ```python
        def replacement(self, inputs):
            a, b, c = inputs[:3]
            # Return the GEMM expression.
            return GEMM(a, b, c, 1.0, 1.0)
        ```

    - **Method 2: Complex scenario (receiving both `inputs` and `match_result`)**

        If you want to use expressions to simplify replacement construction while also reading the attributes of matched nodes, you can receive both parameters:

        ```python
        def replacement(self, inputs, match_result):
            a, b, c = inputs[:3]
            transpose_a = False
            transpose_b = False
            for node in match_result.get_matched_nodes():
                if node.type not in ("MatMul", "BatchMatMulV2"):
                    continue
                try:
                    transpose_a = bool(node.get_attr("transpose_x1"))
                    transpose_b = bool(node.get_attr("transpose_x2"))
                except RuntimeError:
                    pass
                break
            return GEMM(a, b, c, 1.0, 1.0, transpose_a, transpose_b)
        ```

    **Registering a custom fusion pass**

    Use the `@register_fusion_pass` decorator to register the class with GE:

    ```python
    @register_fusion_pass(name="MyPass", stage=PassStage.BEFORE_INFER_SHAPE)
    class MyPass(PatternFusionPass):
        ...
    ```

    name must be unique. [PassStage](../../../api/graph_engine_api/python/ge/passes/PassStage.md) indicates the execution stage. For initial development, it is recommended to use PassStage.BEFORE\_INFER\_SHAPE, because after replacement the graph can still enter GE's subsequent unified shape inference process.

- **Fusion pass development for the single-node replacement (replacing a single node with N nodes) scenario**

    In this scenario, the base class inherited by the pass is DecomposePass. Because the structure to be replaced is a single node, Pattern no longer needs to be defined manually; instead, the operator type is specified through the op\_types parameter during registration:

    ```python
    from ge.passes import DecomposePass, PassStage, register_decompose_pass

    @register_decompose_pass(
        name="PythonMyDecomposePass",
        stage=PassStage.AFTER_INFER_SHAPE,
        op_types=["Conv2D"],
    )
    class PythonMyDecomposePass(DecomposePass):
        def meet_requirements(self, node):
            return node.get_attr("groups") != 1

        def replacement(self, node):
            # Return the replacement graph composed of basic operators
            ...
    ```

    op\_types determines which types of nodes GE hands over to this pass, and meet\_requirements then determines which of those nodes actually need to be replaced.

    **Registering a custom fusion pass**:

    The following is an example of using the [REG\_DECOMPOSE\_PASS](../../../api/graph_engine_api/python/ge/passes/register_decompose_pass.md) decorator to initialize CustomOne2NPass with Conv2D and register it to kAfterInferShape:

    ```python
    @register_decompose_pass(name="CustomOne2NPass", stage=PassStage.AFTER_INFER_SHAPE, op_types=["Conv2D"])
    class CustomOne2NPass(DecomposePass):
        ...

    ```

## Development Example

This section uses the removal of the Add\(x, 0\) structure as an example (using the @pattern decorator) to describe in detail how to modify a Graph through a custom fusion Pass in Python. For the complete example code, see [AddZeroPass Python example](../../../../../examples/fusion_pass/pattern_base_pass/4_add_zero_pass/python/README.md). The source repository also provides examples for the following scenarios for your reference:

- [MatMul+Add Python example](../../../../../examples/fusion_pass/pattern_base_pass/1_fuse_matmul_add_pass/python/README.md)
- [capture tensor Python example](../../../../../examples/fusion_pass/pattern_base_pass/2_fuse_matmul_add_pass_with_capture_tensor/python/README.md)
- [PatternMatcherConfig Python example](../../../../../examples/fusion_pass/pattern_base_pass/3_fuse_matmul_add_pass_with_pattern_matcher_config/python/README.md)
- [DecomposePass Python example](../../../../../examples/fusion_pass/pattern_base_pass/6_decompose_grouped_conv_to_splited_pass/python/README.md)

The graph structures before and after the modification are as follows:

```text
x ----\
       Add ---- out      ==>      x ---- out
0 ----/
```

1. Importing the required modules

    ```python
    from math import fabs

    from ge.graph.types import DataType
    from ge.passes import PassStage, PatternFusionPass, pattern, register_fusion_pass
    ```

2. Implementing the helper functions

    ```python
    def _scalar_value(value):
        while isinstance(value, list):
            if len(value) != 1:
                return None
            value = value[0]
        return value


    def _is_zero(tensor):
        value = _scalar_value(tensor.data)
        if value is None:
            return False
        if tensor.data_type == DataType.DT_FLOAT:
            return fabs(float(value)) < 1e-6
        if tensor.data_type == DataType.DT_DOUBLE:
            return fabs(float(value)) < 1e-15
        if tensor.data_type == DataType.DT_INT32:
            return int(value) == 0
        return False
    ```

3. Implementing the custom Pass

    ```python
    @register_fusion_pass(name="PythonAddZeroPass", stage=PassStage.BEFORE_INFER_SHAPE)  # Register the custom fusion pass and specify the stage at which it is invoked.

    class PythonAddZeroPass(PatternFusionPass):
        @pattern
        def add_zero(self, inputs):
            return inputs[0] + 0  # Define the structure to match: input 0 plus 0.

        def meet_requirements(self, match_result):
            # Check whether the matched constant node is actually 0.
            for node in match_result.get_matched_nodes():
                if node.type != "Const":
                    continue
                return _is_zero(node.get_attr("value"))
            return False

        def replacement(self, inputs):
            return inputs[0]  # Return input 0 directly, which is equivalent to deleting Add.
    ```

    - **@pattern decorator**: inputs\[0\]+0 The expression describes the structure to match: the 0th external input plus a constant 0. GE automatically recognizes this expression and constructs the corresponding Pattern.
    - **meet\_requirements method**: Traverses the nodes in the match result, finds the Const node, and verifies whether its value is 0. Only matches that meet the condition are replaced.
    - **replacement method**: Directly returns inputs\[0\], indicating that the entire Add node is replaced with input 0, achieving the effect of deleting Add\(x, 0\).

## How to Use a Custom Pass

After the custom Pass is implemented, this section describes how to load the Python Pass into the GE compilation phase.

1. Set the CANN environment variable.

    ```bash
    source /usr/local/Ascend/cann/set_env.sh
    ```

    /usr/local/Ascend/ is the default installation path for the root user. Replace it with the actual path.

2. Set the environment variable for loading the Pass.

    GE reads the Python Pass file directory:

    ```bash
    export ASCEND_GE_PY_PASS_PATH=/path/to/my_pass.py
    ```

    It can also point to a directory:

    ```bash
    export ASCEND_GE_PY_PASS_PATH=/path/to/pass_dir/
    ```

    Separate multiple paths with colons:

    ```bash
    export ASCEND_GE_PY_PASS_PATH=/path/to/a.py:/path/to/pass_dir/
    ```

    For details about the environment variable, see [ASCEND\_GE\_PY\_PASS\_PATH](../../../user_guides/env_vars/ASCEND_GE_PY_PASS_PATH.md).

3. Compile the model. The following entry points are supported, but not limited to these, for compiling model files:

    To check whether the custom Pass takes effect, dump the graph before compiling the model: set the [DUMP\_GE\_GRAPH](../../../user_guides/env_vars/DUMP_GE_GRAPH.md) environment variable before execution, and then compile the model using one of the following entry points:

    - Offline compilation: For offline scenarios, use pyatc to trigger compilation. pyatc uses the same command-line parameters as atc, but runs in the current Python interpreter process, which facilitates loading Python Pass. The command is as follows:

        ```bash
        pyatc --model=./model.onnx --framework=5 --soc_version=xxx --output=./model
        ```

        For how to query the value of --soc\_version, see [ATC](../overview/quick_start.md#converting-an-onnx-model-using-atc).

        For details about how to use pyatc, see [Pyatc Interface](../../../api/graph_engine_api/python/ge/pyatc/pyatc_interface.md).

    - Online compilation: Refer to the [Sample Program Execution](../../../../../examples/fusion_pass/pattern_base_pass/4_add_zero_pass/cpp/README.md) section and use the torch\_forward.py script to perform online inference.

## Result Verification

1. Enabling Graph Dump

    It is recommended to enable graph dump for each development:

    ```bash
    export DUMP_GE_GRAPH=1
    ```

2. Viewing Graphs Before and After Fusion

    After the dump environment variable is set and the program execution is complete, graph files such as ge\_onnx\*.pbtxt are generated in the current path. You can obtain the following two graphs (taking the case where the specified Pass execution phase is before InferShape as an example) and then view them using visualization software such as Netron:

    - ge\_onnx\_xxxx\_PreRunBegin.pbtxt: graph before fusion
    - ge\_onnx\_xxxx\_RunCustomPassBeforeInfershape.pbtxt: graph after fusion

    After successful execution, output similar to \(input\_0 appears in the log. The actual name may differ \):

    ```text
    [PythonAddZeroConstValueMatchPass] matched=PythonAddZeroConstValueMatchPass_add_zero_pattern captured=input_0:0
    [PythonAddZeroPass] matched=PythonAddZeroPass_add_zero_pattern captured=input_0:0 const_dtype=DT_FLOAT zero=True
    ```

If the Pass does not match, troubleshoot in the following order:

| Symptom | Possible Cause | Check Method |
| --- | --- | --- |
| The Python file is not loaded | ASCEND_GE_PY_PASS_PATH is not set, the path does not exist, or the suffix is not .py | First check the environment variable and path |
| The class is loaded but the pass is not executed | The registration decorator is not used, or the registration phase is incorrect | Check @register_fusion_pass/@register_decompose_pass |
| The pattern does not match | The operator type, number of inputs, or output boundary is inconsistent | Compare the actual topology in the dump graph |
| The pattern matches but no replacement is performed | meet_requirements returns False | Print the attributes of the matched node |
| The graph is abnormal after replacement | The replacement output does not cover the Tensor required by external consumers | Go back to [Mechanism Document](../../../design/features/fusion_pattern_pass.md) to check the boundary rules |
