# Guide

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:38:31.891Z pushedAt=2026-10-28T03:30:34.506Z nl=2 -->

Custom operators allow developers to integrate their own kernels (such as Ascend C, Triton, or other device-side implementations) into the GE graph compilation and execution process in the form of an independent .so deliverable. This mechanism reuses the performance benefits of graph sinking (online compilation, OM offline models, and static graph address refresh) without modifying the GE framework code.

## How to Start

| Role/What to Do | Relevant Section/Reading Suggestion |
| --- | --- |
| Decision maker: evaluate whether this feature is worth using | Introduction (required, read on first contact with the custom operator entry scenario) |
| Developer: environment not installed yet, confirm version compatibility and pre-checks first | [Environment Preparation](env_preparation.md) (required, read on first contact with the custom operator entry scenario) |
| Developer: want to run a minimal example in 10 minutes (recommended first stop) | [Quick Start](quick_start.md) (required, read on first contact with the custom operator entry scenario) |
| Developer: understand the API system, registration mechanism, and execution scenarios before getting started | [Concepts and Design](concept_and_design.md) (on demand, read when the corresponding scenario/framework/concept is involved) |
| A framework has been selected, integrate following the 5-stage process | [Graph Integration Process](into_graph_flow.md) (on demand, read when the corresponding scenario/framework/concept is involved) |
| Need callback API details (Execute/InferShape/Compile/Serialize) | Reference/[Callback Interface](callback_interface.md) (expert, read when implementing advanced capabilities such as RTC online compilation, OM serialization, and static graph address refresh) |
| Want to know the deliverable directory, .so naming, and deployment specifications | Reference/[Deliverables and OPP Specifications](deliverables_and_opp.md) (on demand, read when the corresponding scenario/framework/concept is involved) |
| Execution failure, incorrect precision, or operator not found | Reference/[Debugging and Common Issues](debug_and_common_issues.md) (on demand, read when the corresponding scenario/framework/concept is involved) |
| Encounter unfamiliar concepts | Reference/[Glossary](glossary.md) (on demand, read when the corresponding scenario/framework/concept is involved) |

## Applicable Scenarios

- When the framework frontend (PyTorch/TensorFlow/ONNX/GE native) introduces an operator that GE does not support, it needs to be executed on the NPU.
- Integrate custom kernel implementations (fused operators, specific optimizations) and reuse the graph sinking capability.
- Deliver as an independent .so, replaceable and updatable at any time independently of the GE version.

**Usage constraints:**

- It does not provide the kernel implementation capability itself (the kernel still needs to be developed using Ascend C/Triton or other means).
- It does not change the registration and scheduling mechanism of existing GE operators; it only provides an entry channel for custom operators.
- Specific interfaces and API names evolve with the CANN/GE version and must be based on the target version.

## Differences from GE Ascend IR Operators

| dimension | Custom Operator | Ascend IR Operator |
| --- | --- | --- |
| Registration Method | REG_OP+REG_AUTO_MAPPING_OP, loaded as a .so deliverable | Internal REG_OP+engine registration in GE |
| Compilation Artifact | Standalone .so, loaded through ASCEND_CUSTOM_OPP_PATH | Released with GE |
| Execution Method | Framework callbacks to the user-implemented Execute/Compile | GE engine scheduling (DNNEngine, etc.) |
| Update Method | Independent of GE, the .so can be replaced at any time | Updated with GE versions |

## Framework Support/Entry Path Comparison

| Frontend | Entry Path | Conversion Mechanism |
| --- | --- | --- |
| PyTorch | FX Graph → GE Graph | TorchAir converter |
| TensorFlow | TF Graph → GE Graph | Adapter automatic mapping |
| ONNX | ONNX Model → GE Graph | Parser + parsing plugin |
| GE native | Direct API graph construction | No conversion layer, direct API graph construction |

## Required Skills

| Skill | Purpose | Required Level |
| --- | --- | --- |
| C++ (including CMake) | Write GE deliverables (.so) and register REG_OP | Required |
| Ascend C or acl basics | Write/call kernels and understand stream/launch | Required |
| PyTorch operator registration (TORCH_LIBRARY) | Bind schema and implementation in PyTorch scenarios | Required for PyTorch scenarios |
| Graph compilation basics (AIR/OM/ATC) | Understand compile-time callbacks (InferShape/Compile/Serialize) | Recommended |
