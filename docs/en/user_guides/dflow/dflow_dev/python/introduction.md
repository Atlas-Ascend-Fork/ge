# Overview

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-11-16T08:55:11.364Z pushedAt=2026-11-17T03:39:11.993Z nl=2 -->

## DataFlow Overview

DataFlow is an application development and execution framework for heterogeneous systems. It provides a set of APIs to help users quickly express business logic. The framework masks the differences between heterogeneous hardware and networking, and improves the overall performance and throughput of applications through an asynchronous pipeline. In addition, DataFlow provides simple and flexible deployment policies, allowing users to conveniently adjust the number and deployment location of each business instance, thereby improving the application's resource utilization and reducing the user's cost.

The following figure shows an example of using DataFlow to enable an asynchronous pipeline to improve throughput:

![](figures/260109164513685.png)

The example application consists of three compute functions. Compute function 1 and compute function 3 are user-defined functions, and compute function 2 is graph execution. In the original flow, the three compute streams are executed serially.

DataFlow uses FlowGraph to carry the entire compute flow. Each compute function corresponds to a FlowNode on the graph, and the node function is carried by FuncProcessPoint or GraphProcessPoint. Nodes are connected through asynchronous data streams. When a FlowGraph has multiple groups of data input, multiple FlowNodes can process different data simultaneously, that is, the original serial execution flow is converted to an asynchronous pipeline execution process.

The relationships between the DataFlow nodes are as follows:

![](figures/260109164713036.png)

- FlowGraph: The graph of DataFlow, which consists of input nodes (FlowData) and compute nodes (FlowNode). It is a DAG (Directed Acyclic Graph), in which the data stream is directed and cyclic expressions are not allowed.
- FlowData: The input node of FlowGraph. FlowData defines the name and type of the input node.
- FlowNode: The compute node of FlowGraph. FlowNode defines the name, number of inputs, and number of outputs of the compute node. The following two types are supported.
  - FuncProcessPoint: The compute processing point of Func, which implements user-defined functions through UDF (User Defined Function).
  - GraphProcessPoint: The compute processing point of Graph, which implements the user's compute logic through IR graph construction.

DataFlow supports users in writing custom processing functions through FuncProcessPoint and GraphProcessPoint, and running them as FlowModel through DataFlow graph construction.

## DataFlow Scenarios

### Multi-Model Concatenated Offload Execution

DataFlow can orchestrate custom compute nodes or models and drive model execution through data. Nodes defined as GraphProcessPoint can be fully offloaded to the device side for execution, and the data transfer between two adjacent GraphProcessPoint nodes is performed between device and device. The above mechanism reduces the interaction of the control plane and data plane between the host and the device, reduces the latency of the entire DataFlow graph execution, thereby improving the overall performance.

As shown in the following figure, the DataFlow graph contains two FuncProcessPoint nodes (UDF0 and UDF1) and two GraphProcessPoint nodes, which are used to load an onnx model and a pb model for inference respectively. The GraphProcessPoint nodes can be fully offloaded to the device for execution. After UDF0 and UDF1 finish processing, the data is transferred to the device, and the execution of the two models and the data transfer are performed entirely on the device.

![](figures/260109165007544.png)

### Online Inference Based on PyTorch Models

DataFlow can be combined with multiple deep learning frameworks for model training or online inference to improve model throughput. Taking PyTorch as an example, when PyTorch is used for online model inference, three steps are usually involved: preprocessing, model inference, and post-processing. Model inference can be further split into multiple serially executed submodels according to the model structure.

According to the above business logic, DataFlow can encapsulate preprocessing, multiple submodel inference, and post-processing into multiple FuncProcessPoint nodes that can be executed asynchronously, and concatenate the data streams between them through asynchronous queues, as shown in the following figure:

![](figures/260109165112546.png)

By converting the original serial execution flow to a FlowGraph, the asynchronous pipeline between different modules can be enabled during multiple rounds of inference, thereby improving the overall throughput of the application.
