# Overview

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-28T12:53:50.076Z pushedAt=2026-10-29T03:25:50.680Z nl=2 -->

## DataFlow Overview

DataFlow is an application development and execution framework for heterogeneous systems. It provides a set of APIs to help you quickly express business logic. The framework shields the differences between heterogeneous hardware and networking, and improves the overall performance and throughput of applications through asynchronous pipelining. In addition, DataFlow provides simple and flexible deployment policies, allowing you to easily adjust the number and deployment location of each service instance, thereby improving resource utilization and reducing usage costs.

The following figure shows an example of using DataFlow to enable asynchronous pipelining for higher throughput:

![](figures/260109164513685.png)

The sample application consists of three computing functions. Computing function 1 and computing function 3 are user-defined functions, and computing function 2 is graph execution. In the original process, the three computing processes are executed serially.

DataFlow uses a FlowGraph to carry the entire computing flow. Each computing function corresponds to a FlowNode on the graph, and the node function is carried by a FuncProcessPoint or GraphProcessPoint. Nodes are connected through asynchronous data flows. When a FlowGraph has multiple groups of data inputs, multiple FlowNodes can process different data simultaneously, that is, the original serial execution process is converted into an asynchronous pipelined execution process.

The relationships between DataFlow nodes are as follows:

![](figures/260109164713036.png)

- FlowGraph: the graph of DataFlow. It consists of input nodes (FlowData) and computing nodes (FlowNode). It is a DAG (Directed Acyclic Graph), in which the data flow is directed and cycles are not allowed.
- FlowData: the input node of a FlowGraph. FlowData defines the name and type of the input node.
- FlowNode: a computation node of FlowGraph. FlowNode defines the name, number of inputs, and number of outputs of the computation node. The following two types are supported.
  - FuncProcessPoint: a computation processing point of Func, which implements user-defined functions through UDF (User Defined Function).
  - GraphProcessPoint: a computation processing point of Graph, which implements user computation logic through IR graph construction.

DataFlow allows users to write custom processing functions through FuncProcessPoint and GraphProcessPoint, and run them as a FlowModel through DataFlow graph construction.

## DataFlow Scenarios

### Offloading Execution of Multiple Chained Models

DataFlow can orchestrate custom compute nodes or models and drive model execution through data. Nodes defined as GraphProcessPoint can be fully offloaded to the device side for execution, and data transfer between two adjacent GraphProcessPoint nodes is performed device-to-device. This mechanism reduces the control-plane and data-plane interactions between the host and the device, lowering the latency of the entire DataFlow graph execution and thereby improving overall performance.

As shown in the following figure, the DataFlow graph contains two FuncProcessPoint nodes (UDF0 and UDF1) and two GraphProcessPoint nodes, which are used to load an ONNX model and a PB model for inference respectively. The GraphProcessPoint nodes can be fully offloaded to the device for execution. After UDF0 and UDF1 finish processing, the data is passed to the device, and the execution of the two models and the data transfer between them are performed entirely on the device.

![](figures/260109165007544.png)

### Online Inference Based on PyTorch Models

DataFlow can be combined with multiple deep learning frameworks for model training or online inference to improve model throughput. Taking PyTorch as an example, online model inference with PyTorch typically involves three steps: preprocessing, model inference, and postprocessing. The model inference step can be further split into multiple serially executed submodels based on the model structure.

Based on the preceding business logic, DataFlow can encapsulate preprocessing, multiple submodel inference steps, and postprocessing into multiple FuncProcessPoints that can be executed asynchronously, and connect the data flows between them through asynchronous queues, as shown in the following figure:

![](figures/260109165112546.png)

By converting the original serial execution process into a FlowGraph, DataFlow enables asynchronous pipelining between different modules during multiple rounds of inference, thereby improving the overall throughput of the application.
