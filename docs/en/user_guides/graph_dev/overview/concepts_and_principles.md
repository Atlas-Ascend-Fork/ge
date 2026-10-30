# Concepts and Principles

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-21T10:03:53.592Z pushedAt=2026-10-28T08:58:47.387Z nl=2 -->

## Tensor/Node/Graph

Ascend IR (Ascend Intermediate Representation) is an abstract data structure dedicated to AI processors and used to express computation flows. Based on the graph engine (GE) capability, algorithm models from mainstream AI frameworks such as PyTorch, TensorFlow, MindSpore, and PaddlePaddle can be uniformly converted into computation graphs represented by Ascend IR. Ascend IR also supports user-defined construction of computation graphs. All subsequent compilation acceleration and optimization are performed based on these computation graphs.

**Figure 1** Computational graph represented by Ascend IR
![Computational graph represented by Ascend IR](../figures/ascend_ir_compute_graph.png "Computational graph represented by Ascend IR")

Ascend IR mainly includes information in three dimensions: Tensor, Node (Operator), and Graph.

### Tensor

A Tensor consists of two parts: the Tensor description and the data. The Tensor description includes the name, dtype, shape, and format information of the Tensor.

**Table 1** TensorDesc attribute description

|Attribute|Definition|
|--|--|
|name|Used to index a Tensor. The names of different Tensors must be unique.|
|shape|Shape of the Tensor, for example, (10,), (1024, 1024), or (2, 3, 4). A shape of (3, 4) indicates that the first dimension has three elements and the second dimension has four elements, that is, a matrix array of three rows and four columns. Format: (i1, i2, ..., in), where i1 to in are all positive integers.|
|dtype|Data type of the Tensor object. Value range: float16, float32, int8, int16, int32, uint8, uint16, bfloat16, bool, and so on.|
|format|Physical layout format of the data. For details, see Data Layout Format.|

### Node

A Node (operator) contains information such as the operator name, type, inputs, outputs, and attributes.

**Table 2** Operator attribute description

|Attribute|Definition|
|--|--|
|name|The name of an operator, used to identify an operator in a graph. Operator names in the same graph must be unique. As shown in Figure 2, Conv1, Pool1, and Conv2 are operator names in the graph. The types of the Conv1 and Conv2 operators are both Convolution, indicating that each performs a convolution operation.|
|type|Each operator in a graph is matched to an implementation based on its operator type. Operators of the same type share the same implementation logic. A graph may contain multiple operators of the same type. For example, both the Conv1 and Conv2 operators in the preceding figure are of the Convolution type.|
|input|The input tensor data of the operator.|
|output|The output tensor data of the operator.|
|Attributes|Defines the behavior and functions of an operator. Common operator attributes include Axis, Weight, and Bias.|

**Figure 2** Computational graph represented by operator names
![Computational graph represented by operator names](../figures/op_name_compute_graph.png "Computational graph represented by operator names")

### Graph

A Graph contains information such as its name, operator list, input operators, and output operators.

**Table 3** Graph attribute description

|Attribute|Definition|
|--|--|
|name|Name of the graph, used to identify a Graph in a network. The names of Graphs in the same network must be unique.|
|Operator list|List of all nodes in the graph.|
|Input operator (input)|Input operator of the graph.|
|Output operator (output)|Output operator of the graph.|

## Working Principle of GE

### Graph Construction

GE provides two methods for constructing graphs:

- Graph construction from models in formats such as ONNX and PB: Through the ATC command-line tool or the Parser API in C++, users map the operators of the frontend framework to CANN operators one by one, thereby parsing the framework model file (in formats such as \*.onnx and \*.pb) into a computation graph represented by Ascend IR.

    **Figure 1**  Parsing files into computational graphs using Parser APIs
    ![Parsing files into computational graphs using Parser APIs](../figures/parser_parse_compute_graph.png "Parsing files into computational graphs using Parser APIs")

    For details about the ATC command-line tool, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).

    For details about constructing a graph using the Parser API, see [Using Parser APIs to Parse the Original Model into a Graph](../construct_graph/parse_model_to_graph_using_parser.md).

- Full construction of a Graph using the graph engine API: Through the graph engine API, users combine computation functions (operators) to construct a computation graph represented by Ascend IR. The following figure shows the basic process of graph construction. For details, see [Using Graph Engine APIs to Construct a New Graph](../construct_graph/construct_graph_using_ge_api.md).

    **Figure 2**  Constructing a new computational graph
    ![Constructing a new computational graph](../figures/build_compute_graph_new.png "Constructing a new computational graph")

### Graph Compilation and Graph Optimization

For the computation graph represented by Ascend IR, GE performs a series of compilation and optimization operations to adapt to the runtime requirements of the underlying hardware, and compiles the graph into an offline model in the .om format. The main process includes:

- Graph preparation: Based on information such as the input tensor information, operator logic, and operator attributes, the output tensor descriptions of operators are inferred in advance, including the tensor shape, data type, and data layout format. In this way, memory can be statically allocated for all tensors during the graph construction preparation phase, avoiding the overhead of dynamic memory allocation. This process is usually called inferShapeAndType and inferFormat. At the same time, hardware-independent, algorithm-level optimizations are performed, including but not limited to constant folding and redundant branch elimination.
- Graph splitting: Operators are classified based on the execution engine (AI Core, AI CPU, etc.), and different subgraphs are formed through splitting, facilitating subsequent targeted optimization on different hardware.
- Graph optimization: The execution performance of the graph is improved through graph optimization methods such as operator fusion. Hardware-independent optimization can be performed, for example, fusing multiple operators into one or several operators to save computation time. Hardware-related optimization can also be performed, for example, shortening the data movement time on hardware memory through UB fusion, thereby improving execution efficiency.

    <!-- npu="950" id1 -->
    **The Ascend 950PR and 950DT products do not support UB fusion.**
    <!-- end id1 -->

- Graph compilation: Runtime resources are allocated based on the computation graph, including memory allocation and stream resource allocation, and an offline model in the .om format is compiled and generated.

    ![CANN architecture diagram](../figures/cann_architecture.png)

### Graph Loading and Graph Execution

Load the compiled offline model file, allocate real runtime resources, and dispatch streams/tasks to the Device for execution. The main process includes:

- Graph loading: Parse the offline model, allocate real memory, and create stream runtime resources.
- Graph execution: Copy input data; dispatch streams/tasks to the Device, where AI Core/AI CPU and other units execute the corresponding operators; after the Device completes computation, return the results to the Host user program.

## Model Sinking Scheduling

### Feature Overview

During AI model execution, the CPU and an AI-specific processor (such as an NPU, also known as an AI processor) usually need to work together. The location of the CPU is called the host side (Host), while the location of the NPU is called the device side (Device). The host side excels at complex logical computation, whereas the device side excels at highly parallel computation. Achieving efficient collaboration between the Host and the Device through an efficient computation scheduling mechanism is key to improving AI model performance and can significantly improve the utilization of heterogeneous system resources.

The Host CPU dispatches the operators in the model to the Device one by one for execution (as indicated by label ① in the following figure). Each operator exists on the execution stream in the form of one or more Tasks, and the Ascend AI processor sequentially fetches and executes the Tasks on the execution stream. The problem with this Host scheduling approach is that it requires frequent interaction between the Host and the Device. In actual training or inference scenarios, a model runs multiple times, and each run triggers the Host to traverse and dispatch all operators in the model once.

**Figure 1** Host scheduling diagram
![Host scheduling diagram](../figures/host_scheduling_diagram.png "Host scheduling diagram")

For models with static shapes, where the input tensor shape remains fixed, the input and output shapes of all operators can be determined at compile time. Combined with the Ascend memory reuse algorithm, model-level memory orchestration can be completed. For static-shape models, Host-side computations such as the Tiling computation of all operators can also be completed in advance at compile time. Therefore, GE provides a static graph sinking scheduling mode, in which the operators in the model are dispatched to the Device in advance as a whole graph during the loading phase. During execution, only one model execution Task needs to be dispatched on the Host side to trigger the model to be scheduled and executed on the Device. Compared with the Host scheduling mode, the sinking scheduling mode can greatly reduce the Host-side scheduling overhead and effectively reduce the interaction between the Host and the Device.

**Figure 2** Static-graph offload scheduling
![Static-graph offload scheduling](../figures/static_graph_scheduling_diagram.png "Static-graph offload scheduling")

### Implementation Principle

Model sinking scheduling consists of two phases: model loading and model execution.

- Model loading: The specific actions of model loading are similar to those of Host scheduling, that is, traversing all operators in the graph and dispatching them to the Device stream as a whole. The difference is that they are not executed immediately after being dispatched to the stream. Model loading is a one-time action and is completed during the first model execution, as shown in process 1 in the preceding figure.
- Model execution: After model loading is complete, a model execution Task can be dispatched to the execution stream in the same way as a single-operator Task. When the Ascend AI processor schedules this Task (as indicated by E in the "execution stream" in the preceding figure), all Tasks in the model are executed (as shown in process 3 in the preceding figure). If the model needs to be run multiple times, only the model execution Task needs to be dispatched multiple times, as shown in process 2 in the preceding figure.

The timing comparison between Host Bound scheduling and model sinking scheduling is shown in the following figure. It can be seen that the start of model sinking execution incurs a head overhead for model dispatch, and the E2E of model sinking execution yields a benefit relative to Host scheduling. The smaller the sinking head overhead of the model, the greater the performance improvement. The Host/Device timing analysis of model sinking scheduling is as follows:

![Time series diagram](../figures/time_series_analysis.png)

Each time a model is dispatched, the Feature Map memory addresses and the input/output memory addresses of the model can be updated. If the Feature Map memory and the model input/output memory of the model are updated, the addresses related to the operators in the model are refreshed during the model sinking header overhead (that is, m\_l\_t in the preceding figure).
