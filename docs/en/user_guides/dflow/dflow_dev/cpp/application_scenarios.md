# Scenarios

<!-- md-trans-meta sourceCommit=b7f5e4a67c5c41d02ea0a83a8f907089d6390ff2 translatedAt=2026-10-28T12:57:33.173Z pushedAt=2026-10-29T03:25:50.688Z nl=2 -->

DataFlow can be used in scenarios where multiple models are chained and offloaded for execution. In such scenarios, DataFlow orchestrates custom compute nodes or models and drives model execution in a data-driven manner. Nodes defined as GraphProcessPoint can be fully offloaded to the device side for execution, and data transfer between two adjacent GraphProcessPoint nodes takes place between devices. This mechanism reduces the control-plane and data-plane interactions between the host and the device, lowering the latency of the entire DataFlow graph execution and thereby improving overall performance.

As shown in the following figure, the DataFlow graph contains two FuncProcessPoint nodes (UDF0 and UDF1) and two GraphProcessPoint nodes, which are used to load an ONNX model and a pb model for inference respectively. The GraphProcessPoint nodes can be fully offloaded to the device side for execution. After UDF0 and UDF1 finish processing, the data is transferred to the device, and the execution of the two models and the data transfer between them are performed entirely on the device. For the sample code, see [Complete Sample Reference](sample_reference.md).

![](figures/260109145241276.png)
