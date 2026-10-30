# AscIR and AscGraph

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-19T12:29:08.199Z pushedAt=2026-10-20T06:49:39.407Z nl=2 -->

## Overview

Ascend C IR (hereinafter referred to as AscIR) is a set of IRs defined based on the Ascend C language, dedicated to the backend work of the automatic fusion software stack. IRs in AscIR correspond to the [Ascend C APIs](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/api/README.md). The type of AscIR is used to distinguish the corresponding Ascend C API; the input, output, and attribute in the AscIR definition are mapped to various parameters of the Ascend C APIs.

As learned from [Fusion Principles](../autofuse/overview.md), if the fusion scope is expressed by a FusedGraph, the FusedGraph contains at least one AscBackend node. One AscBackend node corresponds to one AscGraph, and one AscGraph contains multiple AscIR nodes. AscIR and AscGraph are described separately below.

## AscIR

An AscIR consists of the following parts:

- **Type**: The type determines the computation type of the AscIR and is the fundamental information that distinguishes different IRs.
- **Input and output**: Define the number of inputs and outputs and the meaning of each input and output.
- **Attribute**: Strictly speaking, an attribute is also an input of the AscIR. However, unlike an input, the attribute value must be determined when the AscGraph is constructed, and it is not allowed to change during subsequent execution.

For example, consider the following IR definition:

```c++
REG_ASC_IR(Load)
    .Input("T")
    .Output("T")
    .Attr<Expression>(offset, 0)
    .DataType("T", {DT_FLOAT, DT_FLOAT16});
```

The preceding code defines an IR of the **Load** type. This IR:

- Has one input, whose type can be **DT\_FLOAT** or **DT\_FLOAT16**.
- Has one output, whose type is the same as that of the input.
- Has one attribute named **offset**, whose data type is **int64\_t** and whose default value is **0**.

In addition to the above information, AscIR usually needs to provide the following information: the supported input and output buffer hardware types (GM or UB, which must meet all requirements defined by AscIR when constructing a graph, otherwise errors will occur in subsequent processes). Generally, AscIR includes two types:

- Computation: includes **Load** and **Add**, which is a type of hardware unit that is truly used in the work.
- Buffer: indicates a buffer, for example **Data**, **Output**, and  **Workspace**.

## AscGraph

### Concept Introduction

In the current operator implementation model, the complete computation logic includes a host (usually called the Tiling function) and a device part (usually called the kernel). When an operator is executed, the Tiling function on the host side is run first based on information such as the input shape, produces **TilingData**, and passes it as the input of the kernel to continue executing the device part. On the device side, a single copy of kernel code runs concurrently on multiple hardware units (also called cores or blocks). Therefore, each core is responsible for only a part of the computation of the entire operator. Logically, the Tiling function is responsible for selecting the most efficient kernel implementation for the current shape and computing appropriate strategies such as core splitting for the selected kernel. The result is passed to the kernel through **TilingData** to guide the kernel to run correctly.

The result of automatic fusion is an AscBackend or FusedAscBackend node in the GE graph. FusedAscBackend also carries a subgraph object whose type is ComputeGraph, and a ComputeGraph contains one or more AscBackend nodes. It serves only as a shell node produced by the automatic fusion software stack, while the actual computation logic is stored in the AscGraph attribute of the node. AscGraph uses a directed acyclic graph (DAG) as its basic data structure. Based on symbolic capabilities, AscGraph can fully express the computation logic of an operator (including the core splitting strategy). Codegen generates single-core kernel code based on the intra-core computation part in AscGraph. Auto Tiling generates the Tiling function based on the complete AscGraph. The TilingData definition and the InferShape derivation function are produced based on the symbolic information on AscGraph.

AscGraph has two forms:

- [HintGraph](#hintgraph): the output of the fusion framework, serving as the input of the backend, and describing only the computation logic of the operator.
- [ImplGraph](#implgraph): the result after Auto Schedule processing, serving as the input of the Codegen and Auto Tiling blocks, and describing the complete implementation logic of the operator, including information such as the scheduling strategy, buffer management, and pipeline parallelism.

Although HintGraph and ScheduleResult differ in their expression levels and purposes, they share the same basic graph construction logic and reuse some fields, while each also defines some specific fields. However, **the semantics of the same field remain consistent across different forms** and do not change their meaning because of the form.

#### Structure of AscGraph

AscGraph expresses multi-step computation in multi-level loops: each step of computation corresponds to a node in the graph, and each node is instantiated from AscIR. The directed edges between nodes represent the data transfer relationships, so AscGraph is a DAG. Each node also contains attributes that specify the loop level at which the computation resides.

The **instantiation** process refers to the step of converting AscIR into an AscGraph node, which can be analogized to the creation process from a class to an object. During this process, the inputs, outputs, and attributes of AscIR are all assigned concrete values, making the node executable. For example, if the AscIR type is Cast, the **dst\_dtype** attribute is set during instantiation to indicate the data type to which the input data is to be converted in this type conversion.

The **attribute** field defined by the AscIR specifications. It can exist on the graph or on a node, and can be either a simple value or a container that contains more sub-fields (similar to a struct in C).

#### Node

A node is an entity instantiated from AscIR, and its inputs, outputs, and attributes fully follow the AscIR definition. In AscGraph, each node represents a computation operation and is nested within loop structures for repeated execution.

- **Computation semantics of a node**

    The complete computation semantics of a node consists of the following parts:

  - **Node type**: the computation type of the node, for example, Add represents addition.
  - **Node attributes**: necessary information that supplements the description of the computation logic. For example, the **dst\_type** attribute of a Cast node specifies the target data type to which the input data is to be converted.
  - **Description of inputs and outputs**: for some nodes, the above information of the inputs and outputs is required to jointly describe the computation logic. For example, the Broadcast node relies on the **repeats** and **axis** nodes of the inputs and outputs to determine on which axis broadcasting occurs, and the Cast node relies on the **dtype** attribute of the output to determine the data type to which the input is to be converted.

- **Attributes of a node**

    Each AscGraph node contains the following common attributes that take effect in all forms (HintGraph and ImplGraph):

  - **name**: the node name, which is unique within an AscGraph. If multiple AscGraphs exist (for example, multiple ImplGraphs in the ScheduleResult stage, or multiple AscGraphs in FusedAscBackend in the HintGraph stage), node names are allowed to be duplicated across different AscGraphs.
  - **type**: node type, indicating the AscIR type corresponding to the node.
  - **sched**: defines the execution mode of the node, including the **sched.axis** attribute, which indicates the loop level to which the node belongs. For details, see [Axis](#axis).
  - **Other attributes specific to AscIR**: the node also contains IR-specific attributes defined in [AscIR](#ascir).

- **Input and output attributes**

    The input and output of each node have attributes. AscGraph requires that the input and output attribute values must be identical at both ends of the same edge. Therefore, in most cases, only the output attributes of a node need to be considered, because the input attributes of connected nodes are necessarily identical. The following are common attributes applicable to all forms:

  - **axis**: the number of axes contained in the output. All axes must reference axes already defined globally in AscGraph.
  - **repeats**: the number of data repetitions on each axis, that is, the size of the output on each axis.
  - **stride**: the index stride of the output on each axis.

- **Node execution mode**

    In AscGraph, a node is usually nested in multiple loop levels and executed multiple times. For example, assume that the scheduling information of the Foo node is as follows:

    ```text
    Foo.sched.axis = [z0, z1, z2, z3];
    ```

    This means that the Foo node will be executed in s0 \* s1 \* s2 \* s3 iterations, that is, each loop variable (z0, z1, z2, z3) traverses its corresponding size (s0, s1, s2, s3) in turn. It is equivalent to the following C++ code:

    ```c++
    for (int64_t i0 = 0; i0 < s0; ++i0) {  // Traverse the `z0` axis
        for (int64_t i1 = 0; i1 < s1; ++i1) {  // Traverse the `z1` axis
            for (int64_t i2 = 0; i2 < s2; ++i2) {  // Traverse the `z2` axis
                for (int64_t i3 = 0; i3 < s3; ++i3) {  // Traverse the `z3` axis
                    // Execute the Foo computation
                }
            }
        }
    }
    ```

#### Axis

Axis is an extremely important concept in AscGraph. AscGraph defines both loops and data based on axes. An axis represents a dimension of data and can be identified by the name or ID. Its core attribute **size** describes the length of that dimension. The axis and the symbol of its size are stored on AscGraph as attributes and are graph-level definitions. Therefore, an axis can be referenced by any element in the graph (such as a node) through the axis id, enabling shared understanding and reference of the same dimension.

To create an axis on the graph, for example, create an axis of length s0:

```c++
AscGraph graph;

// The axis length can be a variable named `s0`.
Expression s0 = graph.CreateSizeVar("s0");

// Create an axis named `z0` with length `s0`. The axis `id` is assigned at creation and can be obtained through `z0.id`
Axis &z0 = graph.CreateAxis("z0", s0);
```

In the naming convention of automatic fusion, **the s prefix is generally used to denote size variables**. For example, **s0** and **s1** denote the 0th and 1st size variables, respectively. The letter s is taken from the first letter of **symbol**, indicating that this is a symbolically expressed variable.

Axes are generally named with the **z prefix**, such as **z0** and **z1**, and follow the following correspondence: the size of **z0** is generally represented by **s0**, the size of **z1** by **s1**, and so on. This naming convention ensures that **the mapping between an axis (z) and its size (s) is clear and readable**, making it easy to understand.

To express a loop through axes, for example, the current AscGraph has four axes:

```c++
AscGraph graph;
Expression s0 = graph.CreateSizeVar("s0");
Expression s1 = graph.CreateSizeVar("s1");
Expression s2 = graph.CreateSizeVar("s2");
Expression s3 = graph.CreateSizeVar("s3");

Axis &z0 = graph.CreateAxis("z0", s0);
Axis &z1 = graph.CreateAxis("z1", s1);
Axis &z2 = graph.CreateAxis("z2", s2);
Axis &z3 = graph.CreateAxis("z3", s3);
```

When the loop is z0, z1, z2, z3, it expresses **sequential, full** traversal of these four axes. The equivalent expression in C is:

```c++
for (int64_t i0 = 0; i0 < s0; ++i0) {  // Traverse the `z0` axis, with a traversal length of `s0`, the length of the `z0` axis
    for (int64_t i1 = 0; i1 < s1; ++i1) {  // Traverse the `z1` axis.
        for (int64_t i2 = 0; i2 < s2; ++i2) {  // Traverse the `z2` axis.
            for (int64_t i3 = 0; i3 < s3; ++i3) {  // Traverse the `z3` axis.
            }
        }
    }
}
```

#### Graph Input and Output

AscGraph expresses inputs and outputs through nodes of specific types, and the corresponding AscIR is as follows:

- **AscIR of the Data type** represents the input of the graph.
  - A **Data** AscIR has no input and only one output, corresponding to a certain input of the graph.
  - Its **int32\_t index** attribute is used to indicate the sequence number of this input in the graph.
  - Each Data node corresponds to one graph input, and the node connected to it is the operator that reads this input.

- **AscIR of the Output type** represents the output of the graph.
  - An **Output** AscIR has no output and only one input, corresponding to a certain output of the graph.
  - Its **int index** attribute is used to indicate the sequence number of this output in the graph.
  - Each Output node corresponds to one graph output, and the node connected to it is the final output operator of the entire graph.

**Unlike the inputs and outputs of regular nodes, the output of the Data node and the input of the Output node represent the input and output of AscGraph, respectively. They are the commitments of AscGraph to the outside and must not be modified during the entire backend execution process.**

### HintGraph

HintGraph corresponds to the stage of the backend entry. To be precise, HintGraph is the specification for the input of the Auto Schedule module. In the HintGraph stage, all buffers (including UB and GM) are considered infinite, and each node is an independent loop that completes the processing of its input. In addition, the HintGraph stage does not consider core splitting and assumes that all operators complete their computation on a single core. For example, consider the following HintGraph:

![Illustration](../figures/hintgraph.png)

According to the buffer semantics of the nodes, the Load output, the Add input and output, and the Store input should reside in the UB buffer, while the Data output resides in the GM buffer. However, the UB buffer is finite, so the data output by Data cannot be fully loaded into UB before the subsequent Add computation proceeds. Since the HintGraph stage does not concern itself with buffer size, the figure above is still correct.

In the HintGraph stage, certain requirements and conventions are imposed on AscGraph. When these conventions cannot be satisfied but still need to be expressed, Ascend IR of the FusedAscBackend type can be used to split the expression into multiple AscGraphs:

#### Continuity Conventions

- Explicit Broadcast: If a Broadcast operation occurs during computation, it must be expressed explicitly, that is, a Broadcast operator must be added to the graph.
- Centralized Broadcast: If such an operation occurs, it must be performed immediately after load without any other computation in between.
- Single-axis Broadcast: If multi-axis broadcast occurs, each Broadcast node broadcasts at most one axis.

#### One Set of Loop Axes

An AscGraph can have only one set of loop axes. That is, the **sched.axis** of all nodes on the graph must be identical.

#### keep\_dims Is Always true for Reduction

There is no **keep\_dims** parameter in reduction operations and **keep\_dims** is considered **true** by default.

### ImplGraph

During the process in which Auto Schedule processes the HintGraph, one or more ScheduleResults are generated based on multiple Schedule strategies. Each ScheduleResult shows the complete implementation logic of the operator. The overall computation of a ScheduleResult is split into one or more steps, and each sub-computation step is called a ScheduleGroup, meaning that a schedule strategy is applied independently. Within a ScheduleGroup, each schedule strategy produces a new AscGraph, which is called an ImplGraph.

Each ImplGraph also includes the schedule strategy, inter-core and intra-core splitting methods, buffers, and vectorization.

#### Axis Transformation

An axis can be split into two sub-axes: the outer axis and the inner axis. For example, after splitting the **z0** axis whose **repeats** is **s0**, two axes, **z0Outer** and **z0Inner**, are produced. The product of the **repeats** of the two axes equals **s0**. Therefore, the repeats of the two axes can be expressed as **ceil\(s0/s0I\)** and **s0I**.

Axis splitting is classified into two types: block splitting and tile splitting:

- Block splitting means that after the data is split, it is distributed to multiple cores for parallel execution. The outer axis and inner axis of block splitting are generally named by appending B/b to the original axis name. On an ImplGraph, there can be only one block outer axis. For example, if **z0** is split by Block, the names of the resulting outer axis and inner axis are **z0B** and **z0b**, respectively.
- Tile splitting is ordinary splitting and does not involve special semantics.

Two or more **continuous loop axes** can be merged into one. Continuous loop axes refer to axes that are continuous in **sched.axis**. For example, if **sched.axis** = **\[z0, z1, z2, z3\]**, then **\[z1, z2, z3\]** are continuous axes. If an axis is split, for example, **\[z0, z1T, z1t, z2T, z2t, z3\]**, then **\[z1t, z2T\]** are continuous axes.

If loop axes have been reordered, for example, in the above example, the order becomes **\[z0, z2, z1, z3\]**, then **\[z0, z2\]** are continuous axes. If split axes are reordered, the split axes in the reorder part **cannot** be merged. For example, in the above example, after **z1** is split and reordered, the order becomes **\[z0, z1T, z2, z1t, z3\]**, then **\[z2, z1t\]** and **\[z1t, z3\]** are not continuous axes, and therefore cannot be merged. In this example, the order of **\[z0, z1T\]** is not affected by reordering and can still be merged.

In the axis attributes, the following fields express the transformation relationship of an axis:

- **axis.type**: type of axis transformation from which this axis is produced. If an axis undergoes multiple transformations, this type stores only the transformation type of the last transformation.
  - **kAxisTypeOriginal**: original axis, which has not been split or merged.
  - **kAxisTypeBlockOuter**: outer axis after block splitting.
  - **kAxisTypeBlockInner**: inner axis after block splitting.
  - **kAxisTypeTileOuter**: outer axis after tile splitting.
  - **kAxisTypeTileInner**: inner axis after tile splitting.
  - **kAxisTypeMerged**: axis obtained by merging multiple axes.

- **from**: If the original axis is produced by splitting, **from** stores the axis that was split. If the original axis is produced by merging multiple axes, **from** stores the axes before merging.
- **split\_pair**: If the original axis is produced by splitting, **split\_pair** stores the other axis produced by the same split.

#### Vectorization and Loop Axes

In ImplGraph, the definition of an axis is extended to express three meanings. Consider an API call:

```python
// There are four axes `z0, z1, z2, z3` with lengths `s0, s1, s2, s3`, and `q + index` is used for indexing.
for q0 in range(s0):
    buffer[s1 * s2 * s3];
    for q1 in range(s1):
        // inputs contains four axes: q0, q1, q2, q3. The data of the two axes `q2, q3` is computed in this call.
        buffer[index] = CalcApi(inputs[q0][q1])
```

The preceding call contains the following information:

- Number of loop levels: q0 and q1
- Data length of one computation: s2 \* s3
- Length of the buffer for storing data after computation. In the example, the buffer contains three axes z1, z2, and z3, so its length is the product of the lengths of the three axes.

This is expressed through the following fields:

|Position|Attribute key|Description|Value in this example|
|--|--|--|--|
|On the node|**sched.axis**|All axes on the node|[z0, z1, z2, z3]|
|On the node|**sched.loop_axis**|Loop stop axis. The stop axis and the axes before it form the loop part, and the part after the stop axis is the data volume of one computation|z1|
|On the output|**vectorized_axis**|Length of the buffer for storing data|[z1, z2, z3]|
|On the output|**vectorized_stride**|Stride for storing data|[s2*s3, s3, 1]|
