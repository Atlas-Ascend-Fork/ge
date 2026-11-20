# Overview

<!-- md-trans-meta sourceCommit=ba12e592e8f84bc59709464bd3ad6ca010498d8b translatedAt=2026-11-06T10:42:17.620Z pushedAt=2026-11-13T02:43:08.816Z nl=2 -->

## What Is AutoFuse

AutoFuse is an automatic operator fusion component for Ascend series chips in the CANN ecosystem. It receives the fused subgraphs and unified IR produced by graph compilation components such as GE and Inductor after graph transformation, lowering, and fusion scope determination, and then completes scheduling optimization, tiling solving, and code generation within the determined fusion scope, ultimately outputting high-performance Ascend C fused operators. By fusing multiple originally independent operators into a single kernel, AutoFuse reduces GM read/write of intermediate results, the number of kernel launches, and host-device scheduling overhead, thereby significantly improving model execution performance on Ascend NPUs.

The benefit principle is shown in the following figure. Automatic fusion merges multiple operators into a single operator, which theoretically yields benefits in both MTE data movement and dynamic shape scheduling overhead. For recommended networks with small shapes and MTE-bound characteristics, positive benefits can generally be obtained.

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
- Atlas A2 series products
<!-- end id3 -->

## System Architecture

The overall logical structure of automatic fusion is shown in the following figure:

**Figure 2** System architecture<a id="fig1"></a>
![Figure 2 example](../figures/autofusion_arch.png "Technical route")

As shown in the preceding figure, the automatic fusion solution is built on AscendLoopIR (an IR modeled for the Ascend C programming language), which is the unified underlying IR of Ascend NPUs, together with the supporting scheduling and code generation capabilities. It provides two fusion implementation paths:

- **GE path**: Based on Huawei's self-developed GE framework, this path focuses on Ascend NPU affinity. GE performs symbolization, lowering, and fusion scope determination, while AutoFuse handles backend scheduling, splitting, and code generation.
- **Inductor path**: This path integrates with PyTorch Inductor and focuses on ecosystem adaptation. It reuses Inductor's fusion scope identification capability, while backend processing is still performed by AutoFuse.

The following describes the role of each component in detail.

### Frontend Adaptation

Frontend adaptation converts model graphs from mainstream deep learning frameworks such as PyTorch and TensorFlow into a graph IR that AutoFuse can process.

- **GE path**: In online scenarios, graph representations such as AtenIR and GraphDef are converted into AscendIR through TorchAir or the TensorFlow Adapter, and then enter the GE graph compilation process. In offline scenarios, models in formats such as TensorFlow and ONNX are parsed into AscendIR through the built-in ATC Parser.

- **Inductor path**: AtenIR of PyTorch is converted into InductorIR, following the PyTorch Inductor path. For details about the TorchNPU module, see the [PyTorch project](https://gitcode.com/Ascend/pytorch).

### Graph Engine

As the frontend of AutoFuse, Graph Engine performs symbolic derivation, lowering, and CanFuse fusion condition evaluation on the AscendIR graph to determine the scope of operators that can be fused and the computational expression after fusion.

| Step                                          | Responsibility                                               |
| --------------------------------------------- | ------------------------------------------------------------ |
| **[Symbolization](symbolization.md)**         | Expresses operator shapes with symbols to enhance dynamic shape processing, and provides simplification, derivation, and guard capabilities, delivering key information for subsequent loop-axis merging and memory optimization. |
| **[Lowering](lowering.md)**                   | Converts high-level AscendIR into low-level AscendLoopIR to express computational logic close to Ascend C semantics, and determines the fusion structure and data dependencies. |
| **[CanFuse (Fusion Strategy)](fusion_strategy.md)** | Determines the fusion boundary from three layers: semantic correctness, expressibility, and resource budget. |

### AutoFuse

As the backend of the upper-layer GE or Inductor, AutoFuse is the core of automatic fusion compilation. It receives the determined fusion scope, completes the scheduling, code generation, and tiling strategy solving of the fusion kernel through three core modules, namely Schedule, Codegen, and Auto Tiling, and provides operator API support with the help of the Ascend C API.

| Module                            | Responsibility                                               |
| --------------------------------- | ------------------------------------------------------------ |
| **[Schedule](schedule.md)**       | Scheduling strategy generation: computation reordering, loop fusion, parallel optimization, memory optimization, and multi-template generation. |
| **[Codegen](codegen.md)**         | Code generation: parses the scheduling graph and generates host-side and device-side code. |
| **[Auto Tiling](auto_tiling.md)** | Tiling solving: solves the tile size and core-splitting strategy under UB constraints, evaluates the performance of splitting schemes, and selects the appropriate template and splitting strategy. |

### Compiling and Running

The host and device source code generated by AutoFuse is further compiled by the [Bisheng Compiler](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/compiler/BishengCompiler/atlas_bisheng_10_0001.html) into a host-side shared library and device-side kernel binaries. At runtime, the upper-layer framework prepares the Tiling parameters based on the actual inputs and launches the fused kernel. **Runtime** is responsible for device resource management, kernel dispatch, and execution. For an operator chain that originally requires multiple kernels, Fusion generally reduces the number of kernel launches and the global memory reads/writes of intermediate Tensors, thereby lowering data transfer and scheduling overhead and improving the hardware resource utilization of the Ascend AI processor and the model execution performance.

## Key Technical Solutions

AutoFuse classifies network operators into two categories based on computational characteristics: one category consists of basic computation types such as Elemwise, Broadcast, and View (Transpose, Slice, Split); the other category consists of computation types such as Reduce, Concat, and MatMul that extend fusion capabilities on top of the basic computation types. All extended fusion capabilities must support fusion with the basic computation types.

### Operator Type

The following table lists the main supported operator types and their corresponding compute units:

| Operator Type | Compute Unit | Typical Operator Example | Applicable Scenario | Description |
| :------------ | :--------- | :------------------------------ | ------------------------------------------------------------ | ------------- |
| **Elemwise** | Vector | Add, Mul, Abs, Exp, Relu, Cast | GE path, Inductor path | Element-wise computation, where each output element corresponds one-to-one with an input element. |
| **Broadcast** | Vector | BroadcastTo, BiasAdd | GE path, Inductor path | Broadcast computation, which expands data of a smaller shape along the broadcast axis and then performs element-wise computation. |
| **Concat** | MTE/Vector | Concat | GE path, Inductor path | Concatenation computation, which concatenates multiple tensors into one tensor along a specified axis. |
| **MatMul** | Cube | MatMul | Inductor path | Matrix computation, including matrix multiplication and convolution. |
| **Reduce** | Vector | ReduceSum, ReduceMax, ReduceMin | GE path, Inductor path | Reduction computation, which aggregates multiple elements along a specified axis. |
| **General Norm** | Vector | LayerNorm, RMSNorm | Inductor path | A normalization computation pattern formed by combining computations such as same-axis Reduce, Broadcast, and Elemwise, rather than a single operator. |
| **View** | MTE/Vector | Transpose, Slice, Split | Inductor path | View transformation, which changes the logical shape, axis order, or splitting method of data. |

### Supported Fusion Capabilities

AutoFuse currently supports two types of fusion: VV and CV.

| Fusion Type                       | Fusion-Capable Operator Types                                               |
| :----------------------------- | :----------------------------------------------------------- |
| **VV Fusion** (Vector + Vector) | Supports the fusion of Vector operators such as Elemwise, Broadcast, View (including Transpose, Slice, and Split), Reduce, and Concat. |
| **CV Fusion** (Cube + Vector)   | Supports the fusion of Cube operators with Vector operators.                     |

## Fusion Principle

The implementation of automatic fusion consists of two parts: automatically determining the fusion scope, and automatically generating the fused kernel source code and kernel binary based on the fusion scope. The former is called the automatic fusion frontend, and the latter is called the automatic fusion backend (corresponding to AutoFuse and the subsequent parts in [Figure 2](#fig1)):

The frontend mainly determines which operators can be fused based on certain rules or configurations, and determines the fusion scope of the fused operators. The fusion scope is expressed by a FusedGraph, as shown in [Figure 3](#fig2). A FusedGraph contains \>=1 AscBackend nodes. An AscBackend can be understood as an Ascend IR operator similar to ge::op::partitionedcall, carrying a subgraph object. An AscBackend node carries an AscGraph attribute, and an AscGraph contains multiple AscIR nodes. For details about AscIR and AscGraph, see [AscIR and AscGraph](../appendix/ascir_and_ascgraph.md).

**Figure 3**  FusedGraph<a id="fig2"></a>
![Figure 3 example](../figures/fused_graph.png "FusedGraph")

**Figure 4**  AscGraph corresponding to AscBackend
![Figure 4 example](../figures/ascbackend_ascgraph.png "AscGraph corresponding to AscBackend")

The implementation of the backend includes Schedule, Codegen, Auto Tiling, and so on. After the backend receives the FusedGraph, Schedule first generates the corresponding TilingGroup for compute and data movement nodes respectively, and builds a normalized TilingGroup applicable to the entire fused operator through the TilingGroup merging strategy. Finally, based on the normalized TilingGroup, the Tiling strategy of the FusedGraph is generated. After Codegen implementation and Auto Tiling, the kernel source code, tiling\_func source code, and tiling\_data source code are generated, and the host and Device deliverables are compiled and generated. For details about how automatic fusion takes effect, see [Enabling AutoFuse](autofuse_enable.md). For the implementation principles of [Schedule](schedule.md), [Codegen](codegen.md), and [Auto Tiling](auto_tiling.md), see the corresponding sections.
