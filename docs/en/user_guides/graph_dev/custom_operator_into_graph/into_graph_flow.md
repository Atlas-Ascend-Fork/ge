# Graph Integration Process

<!-- md-trans-meta sourceCommit=557fc263a581ed94fc4bb5447e90358eea64338d translatedAt=2026-11-06T11:12:14.887Z pushedAt=2026-11-13T11:13:30.104Z nl=2 -->

Before performing graph integration, you need to first identify the execution scenario and determine the frontend framework, and then carry out specific operations according to the different stages.

## 1. Locating the Execution Scenario

| Scenario | Description | Interface Combination | Applicable Framework |
| --- | --- | --- | --- |
| Scenario 1: Dynamic graph online execution | The kernel is already compiled and is directly added to the graph for execution. | EagerExecuteOp + ShapeInferOp (optional) | All frameworks |
| Scenario 2: Online compilation + online execution | The kernel is compiled by RTC at graph compile time and then executed. | EagerExecuteOp + CompilableOp + ShapeInferOp (optional) | All frameworks |
| Scenario 3: Offline OM model offloading | The compilation artifacts are serialized into the OM and loaded for restoration. | EagerExecuteOp+CompilableOp+ShapeInferOp+PortableOp | All frameworks |
| Scenario 4: Data-dependent shape | The output shape depends on runtime data. | EagerExecuteOp+ShapeInferOp | All frameworks |

For details about the execution scenarios, see [Execution Scenarios](concept_and_design.md#execution-scenarios). The mapping between execution scenarios and callback interfaces is as follows. For details about the callback interfaces, see [Callback Interface](callback_interface.md). Only the trigger timing is listed here.

| Callback Interface | Trigger Time | Applicable Scenario |
| --- | --- | --- |
| InferShape | Shape inference at compile time | All scenarios (optional) |
| InferDataType | Data type inference at compile time | All scenarios (optional) |
| Compile | RTC kernel compilation at compile time | Scenario 2/Scenario 3 |
| Serialize | Writing to OM during ATC compilation | Scenario 3 |
| Execute | Kernel execution at runtime | All scenarios (must be implemented) |
| Deserialize | Restoring binary when OM is loaded | Scenario 3 |
| UpdateHostArgs | Refreshing when I/O addresses change | Static graph offloading scenario |

## 2. Selecting a Framework

| Frontend | Graph input path | Conversion mechanism | Manual REG_OP? | Key Deliverable | Documentation |
| --- | --- | --- | --- | --- | --- |
| PyTorch | FX Graph → GE Graph | TorchAir converter | Yes | converter+GE .so | [PyTorch Graph Integration](pytorch_into_graph.md) |
| TensorFlow | TF Graph → GE Graph | Automatic mapping by Adapter | No (auto-generated) | npu_supported_ops.json+GE .so | [TensorFlow Graph Integration](tf_into_graph.md) |
| ONNX | ONNX Model → GE Graph | Parser + parsing plugin | Yes | libonnx_parser_plugin.so+GE .so | [ONNX Graph Integration](onnx_into_graph.md) |
| GE Native | Direct API graph construction | No conversion layer | Yes | GE .so | [GE Native Graph Construction for Graph Integration](ge_native_into_graph.md) |

For the deliverable list of each framework, see [Deliverables and OPP Specifications](deliverables_and_opp.md).

## 3. Overview of the Five Common Stages

The process of integrating a custom operator into a graph is divided into five progressive stages that are common to all frameworks (the only difference lies in the form of deliverables). The responsibilities of each stage and the involved deliverables are as follows:

| Stage | Responsibility | Deliverables Involved |
| --- | --- | --- |
| Stage 1: operator definition | Declares operator metadata (inputs, outputs, and attributes) | Proto header files, etc. |
| Stage 2: frontend integration | Maps the frontend operator type to the GE type | Parses the plugin .so, etc. |
| Stage 3: graph construction and integration | Converts the frontend computation graph into a GE computation graph | No additional deliverable (completed by the framework) |
| Stage 4: graph compilation | Performs compile-time inference and compilation | Callback implementation in the GE deliverable .so |
| Stage 5: graph execution | Kernel execution at runtime | Callback implementation in the GE deliverable .so |

The relationship between each stage and [1. Locating the Execution Scenario](#1-locating-the-execution-scenario) is shown in the following table:

| Execution Scenario | Stages 1-3 | Stage 4 | Stage 5 |
| --- | --- | --- | --- |
| Scenario 1: Dynamic graph online execution | Must be completed | Can be skipped | Execute |
| Scenario 2: Online compilation + execution | Must be completed | Compile | Execute |
| Scenario 3: Offline OM offloading | Must be completed | Compile+Serialize | Deserialize+Execute |
| Scenario 4: Data-dependent shape | Must be completed | InferShape (upper bound) | Execute+shape write-back |

## 4. Deliverable Quick Reference

For the complete directory tree of each framework, see [Deliverable and OPP Specifications](deliverables_and_opp.md).
