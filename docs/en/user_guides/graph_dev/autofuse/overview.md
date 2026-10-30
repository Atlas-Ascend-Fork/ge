# Overview

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-21T09:05:09.562Z pushedAt=2026-10-28T03:30:34.448Z nl=2 -->

## What Is AutoFuse

AutoFuse is an automatic operator fusion component in the CANN ecosystem for Ascend chips. It receives the fusion subgraphs and unified IR produced by graph compilation components such as GE and Inductor after graph transformation, lowering, and fusion scope determination, and completes scheduling optimization, Tiling solving, and code generation within the determined fusion scope, ultimately outputting high-performance Ascend C fusion operators. By fusing multiple operators that originally execute independently into a single kernel, AutoFuse reduces GM read/write of intermediate results, the number of kernel launches, and host-device scheduling overhead, thereby significantly improving model execution performance on Ascend NPUs.

The benefit principle is shown in the following figure. Automatic fusion merges multiple operators into a single operator, which theoretically yields certain benefits in both MTE data movement and dynamic Shape scheduling overhead. For recommendation networks with small Shapes and MTE-bound workloads, positive benefits can generally be obtained.

**Figure 1** Benefit principle
![Figure 1 example](../figures/benefit_principle.png "Benefit principle")

<!-- npu="950,A3,910b" id4 -->
The automatic fusion feature supports only the following product models:
<!-- end id4 -->

<!-- npu="950" id1 -->

- Ascend 950PR/Ascend 950DT
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products
<!-- end id3 -->

## System Architecture

The overall logical structure of automatic fusion is shown in the following figure:

**Figure 2** System architecture<a id="fig1"></a>
![Figure 2](../figures/autofusion_arch.png "System architecture")

As shown in the preceding figure, the automatic fusion solution is built on the unified AscendLoopIR (an IR modeled for the Ascend C programming language) at the bottom layer of Ascend NPUs, together with the supporting Schedule and code generation capabilities, and constructs two fusion implementation paths:

- **GE path**: Based on Huawei's self-developed GE framework, this path focuses on Ascend NPU affinity. GE performs symbolization, lowering, and fusion scope determination, while AutoFuse handles backend scheduling, tiling, and code generation.
- **Inductor path**: This path integrates with PyTorch Inductor and focuses on ecosystem adaptation. It reuses Inductor's fusion scope identification capability, while backend processing is still performed by AutoFuse.

The following describes the role of each component in detail.

### Frontend Adaptation

Frontend adaptation converts model graphs from mainstream deep learning frameworks such as PyTorch and TensorFlow into a graph IR that AutoFuse can process.

- **GE path**: In online scenarios, graph representations such as AtenIR and GraphDef are converted into AscendIR through TorchAir or TensorFlow Adapter, and then enter the GE graph compilation process. In offline scenarios, models in formats such as TensorFlow and ONNX are parsed into AscendIR through the built-in Parser of ATC.

- **Inductor path**: The AtenIR of PyTorch is converted into InductorIR, using the PyTorch Inductor path. For details about the TorchNPU module, see [PyTorch Project](https://gitcode.com/Ascend/pytorch).

### Graph Engine

As the frontend of AutoFuse, Graph Engine performs symbolic derivation, lowering, and CanFuse fusion condition judgment on AscendIR graphs to determine the fusion scope of operators and the computational expression after fusion.

| Step                                          | Responsibility                                               |
| --------------------------------------------- | ------------------------------------------------------------ |
| **[Symbolization](symbolization.md)**         | Expresses operator shapes with symbols to enhance dynamic shape processing capabilities, and provides simplification, derivation, and guard functions, delivering key information for subsequent loop axis merging and memory optimization. |
| **[Lowering](lowering.md)**                   | Converts high-level AscendIR into low-level AscendLoopIR to express computational logic close to Ascend C semantics, and determines the fusion structure and data dependencies. |
| **[CanFuse (Fusion Strategy)](fusion_strategy.md)** | Determines the fusion boundary from three layers: semantic correctness, expressibility, and resource budget. |

### AutoFuse

As the backend of the upper-layer GE or Inductor, AutoFuse is the core of automatic fusion compilation. It receives the determined fusion scope, completes the scheduling, code generation, and tiling strategy solving of the fused kernel through three core modules, namely Schedule, Codegen, and Auto Tiling, and provides operator interface support with the help of the Ascend C API.

| Module                              | Responsibility                                                         |
| --------------------------------- | ------------------------------------------------------------ |
| **[Schedule](schedule.md)**       | Scheduling strategy generation: computation reordering, loop fusion, parallel optimization, memory optimization, and multi-template generation. |
| **[Codegen](codegen.md)**         | Code generation: parses the scheduling graph and generates host-side and device-side code.             |
| **[Auto Tiling](auto_tiling.md)** | Tiling solving: solves the tile size and core partitioning strategy under UB constraints, evaluates the performance of partitioning schemes, and selects the appropriate template and partitioning strategy. |

### Compiling and Running

The Host and Device source code generated by AutoFuse is further compiled by the [Bisheng Compiler](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/compiler/BishengCompiler/atlas_bisheng_10_0001.html) into a Host-side shared library and a Device-side kernel binary. At runtime, the upper-layer framework prepares Tiling parameters based on the actual inputs and launches the fused kernel. **Runtime** is responsible for device resource management, kernel dispatch, and execution. For an operator chain that originally requires multiple kernels, Fusion typically reduces the number of kernel launches and the global memory reads/writes of intermediate Tensors, thereby lowering data transfer and scheduling overhead and improving the hardware resource utilization of Ascend AI Processors and model execution performance.

## Key Technical Solutions

AutoFuse classifies network operators into two categories based on their computational characteristics: one category consists of basic computation types such as Elemwise, Broadcast, and View (Transpose, Slice, Split); the other category consists of computation types that extend fusion capabilities on top of the basic computation types, such as Reduce, Concat, and MatMul. All extended fusion capabilities must support fusion with the basic computation types.

### Operator Type

The following table lists the main supported operator types and their corresponding compute units:

| Operator Type | Compute Unit | Typical Operator Examples | Applicable Scenarios | Description |
| :------------ | :--------- | :------------------------------ | ------------------------------------------------------------ | ------------- |
| **Elemwise** | Vector | Add, Mul, Abs, Exp, Relu, Cast | **GE** path, **Inductor** path | Element-wise computation, where each output element corresponds one-to-one with an input element. |
| **Broadcast** | Vector | BroadcastTo, BiasAdd | **GE** path, **Inductor** path | Broadcast computation, which expands data of a smaller shape along the broadcast axis and then performs element-wise computation. |
| **Concat** | MTE/Vector | Concat | **GE** path, **Inductor** path | Concatenation computation, which concatenates multiple tensors into one tensor along a specified axis. |
| **MatMul** | Cube | MatMul | Inductor path | Cube computation, including cube multiplication and convolution. |
| **Reduce** | Vector | ReduceSum, ReduceMax, ReduceMin | **GE** path, **Inductor** path | Reduction computation, which aggregates multiple elements along a specified axis. |
| **Generalized Norm** | Vector | LayerNorm, RMSNorm | Inductor path | A normalization computation pattern formed by combining same-axis Reduce, Broadcast, and Elemwise computations, rather than a single operator. |
| **View** | MTE/Vector | Transpose, Slice, Split | Inductor path | View transformation, which changes the logical shape, axis order, or splitting method of data. |

### Supported Fusion Capabilities

AutoFuse currently supports two fusion types: VV and CV.

| Fusion Type | Fusable Operator Types |
| :----------------------------- | :----------------------------------------------------------- |
| **VV Fusion** (Vector + Vector) | Supports the fusion of Vector operators such as Elemwise, Broadcast, View (including Transpose, Slice, and Split), Reduce, and Concat. |
| **CV Fusion** (Cube + Vector) | Supports the fusion of Cube operators with Vector operators. |

## Fusion Principle

Automatic fusion is implemented in two parts: automatically determining the Fusion Scope, and automatically generating the fused kernel source code and kernel binary based on the Fusion Scope. The former is called the automatic fusion frontend, and the latter is called the automatic fusion backend (corresponding to AutoFuse and the subsequent parts in [Figure 2](#fig1)):

The frontend mainly determines, based on certain rules or configurations, which operators can be fused, and determines the Fusion Scope of the fused operators. The Fusion Scope is expressed by a FusedGraph. As shown in [Figure 3](#fig2) below, a FusedGraph contains \>=1 AscBackend nodes. An AscBackend can be understood as an Ascend IR operator similar to ge::op::partitionedcall, carrying a subgraph object. An AscBackend node carries an AscGraph attribute, and an AscGraph contains multiple AscIR nodes. For details about AscIR and AscGraph, see [AscIR and AscGraph](../appendix/ascir_and_ascgraph.md).

**Figure 3**  FusedGraph<a id="fig2"></a>
![Figure 3 example](../figures/fused_graph.png "FusedGraph")

**Figure 4**  AscGraph corresponding to AscBackend
![Figure 4 example](../figures/ascbackend_ascgraph.png "AscGraph corresponding to AscBackend")

The backend implementation involves  the Schedule, Codegen, and Auto Tiling modules. After the backend receives the FusedGraph, Schedule first generates the corresponding TilingGroup for compute and data-movement nodes respectively, and builds a normalized TilingGroup applicable to the entire fused operator through the TilingGroup merging strategy. Finally, based on this normalized TilingGroup, the Tiling strategy of the FusedGraph is generated. Through Codegen implementation and Auto Tiling, the kernel source code, tiling\_func source code, and tiling\_data source code are generated, and the host and device deliverables are compiled. For details about how automatic fusion takes effect, see [Enabling AutoFuse](autofuse_enable.md). For the implementation principles of [Schedule](schedule.md), [Codegen](codegen.md), and [Auto Tiling](auto_tiling.md), see the corresponding sections.
