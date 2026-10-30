# Schedule

<!-- md-trans-meta sourceCommit=cc5ba9391389e272005d4ade3dda4ccce784361d translatedAt=2026-10-21T09:12:19.868Z pushedAt=2026-10-28T03:30:34.456Z nl=2 -->

The Schedule module of automatic fusion is the core component that connects computation definition with efficient code generation. Its key capability lies in flexibly reordering and optimizing the computation process: without changing the computation semantics, it flexibly adjusts the computation implementation through computation reordering and scheduling primitive invocation to achieve better performance. The optimization methods include loop merging, solution space generation, parallel optimization, memory optimization, and multi-template generation.

After a user defines a [HintGraph](../appendix/ascir_and_ascgraph.md#hintgraph) that describes the Scalar computation logic through AscIR, the Schedule module schedules and optimizes the computation based on hardware characteristics, generating multiple [ImplGraph](../appendix/ascir_and_ascgraph.md#implgraph) that express the tiling and memory relationships. This provides a foundation for the Codegen and Auto Tiling modules, supporting them in generating high-performance kernel code.

## Loop Merging

Loop merging is an important loop transformation technique. Its core purpose is to reduce the number of memory accesses, lower control overhead, improve data locality, and pave the way for subsequent optimizations by restructuring the loop structure, ultimately improving program execution efficiency without changing the computation results.

For example, assume the following two-level loop:

```python
for i in range(N):
    for j in range(M):
        C[i][j] = A[i][j] + B[i][j]
```

Add is an elementwise operation, so it can be merged into a single linear loop:

```python
for fused in range(N * M):
    i = fused // M
    j = fused % M
    C[i][j] = A[i][j] + B[i][j]
```

The corresponding AscIR expression is as follows:

```text
z0 = graph.create_axis("z0", N)
z1 = graph.create_axis("z1", M)
Load load0("load_0");
load0.x = data0.y;
load0.attr.sched.axis = {z0.id, z1.id};
load0.y.axis = {z0.id, z1.id};
load0.y.repeats = {N, M};
load0.y.strides = {M, 1};

Load load1("load_1");
load1.x = data1.y;
load1.attr.sched.axis = {z0.id, z1.id};
load1.y.axis = {z0.id, z1.id};
load1.y.repeats = {N, M};
load1.y.strides = {M, 1};

Add add("add");
add.x1 = load0.y;
add.x2 = load1.y;
add.attr.sched.axis = {z0.id, z1.id};
add.y.axis = {z0.id, z1.id};
add.y.repeats = {N, M};
add.y.strides = {M, 1};
```

After loop merging:

```text
Load load0("load_0");
load0.x = data0.y;
load0.attr.sched.axis = {z0z1.id};
load0.y.axis = {z0z1.id};
load0.y.repeats = {N * M};
load0.y.strides = {1};

Load load1("load_1");
load1.x = data1.y;
load1.attr.sched.axis = {z0z1.id};
load1.y.axis = {z0z1.id};
load1.y.repeats = {N * M};
load1.y.strides = {1};

Add add("add");
add.x1 = load0.y;
add.x2 = load1.y;
add.attr.sched.axis = {z0z1.id};
add.y.axis = {z0z1.id};
add.y.repeats = {N * M};
add.y.strides = {1};
```

## Generating TilingCase

In auto fusion technology, tiling solution space generation is a key step in achieving efficient computation scheduling. Its core objective is to provide diverse tiling strategy options for complex computation tasks, so that the subsequent optimizer can select the optimal solution from them. In simple terms, the process of tiling solution space generation can be understood as a systematic enumeration of the "tiling possibilities" for input data or computation tasks, and each solution space is called a TilingCase.

The design of tiling methods is closely related to the implementation characteristics of operators. To systematically enumerate tiling strategies for diverse operators, operators are first abstracted into 9 compute\_type categories based on their implementation characteristics (such as the compute and view operator categories shown in the following figure). Operators in the same compute\_type category share similar computation logic and data access patterns, and therefore can share a common tiling strategy framework.

**Figure 1** Tiling strategy
![Figure 1](../figures/tiling_split_strategy.png "Tiling strategy")

To concretize this strategy framework, we normalize and group the axes of operators, uniformly dividing all axes into three dimension sets: (xgroup, ygroup, rgroup). The specific definitions are as follows:

- **xgroup**: A group designed specifically for view-type operators such as Concat. Taking Concat as an example, the Concat axis is used for division: the axes before the Concat axis are assigned to xgroup, and the Concat axis and the axes after it are assigned to ygroup.
- **ygroup**: Corresponds to the loop axis grouping of operators such as Elementwise and Broadcast.
- **rgroup**: Reduce-type operations usually have special tiling requirements for the reduce axis, so all reduce axes are placed separately into rgroup.

> [!NOTE]
>
> The core reason for introducing xgroup, ygroup, and rgroup is to support the "dual tiling" requirement in complex scenarios. For example, in a computation graph containing reduce, ygroup controls the loop tiling of elementwise, while the axes in rgroup control the loop tiling in the reduce direction.

After completing the axis grouping of individual operators, the grouping strategies of all operators in the computation graph are merged through preset merge rules (Merge). The merge result serves as a unified tiling strategy applicable to all operators in the entire graph, providing the basis for subsequent solution space generation.

Through the grouping and merging mechanisms described above, two core functions can be implemented:

- Filter out the tiling methods applicable to all nodes in the computation graph to form valid TilingCases.
- Verify the fusibility of two graphs by determining whether the tiling groups of different AscGraphs can be merged successfully.

The following case is used to introduce the principle of generating TilingCases through tiling group merging:

```text
z0 = graph.create_axis("z0", s0)
z1 = graph.create_axis("z1", s1)
data = ascir.ops.Data('data', graph)
data.y.dtype = ascir.dtypes.float32
# Declare the load operator.
load = ascir.ops.Load('load')
load.attr.sched.axis = [z0, z1] # Scheduling axes
load.x = data.y
load.y.axis = [z0, z1] # Output axes of the tensor
load.y.size = [s0, s1] # Output size of the tensor
load.y.strides = [s1, 1] # Output strides of the tensor
# Declare the abs operator.
abs = ascir.ops.Abs('abs')
abs.attr.sched.axis = [z0, z1]
abs.x = load.y
abs.y.axis = [z0, z1]
abs.y.size = [s0, s1]
abs.y.strides = [s1, 1]
# Declare the max operator
max = ascir.ops.Max('max')
max.attr.sched.axis = [z0, z1]
max.x = abs.y
max.y.axis = [z0, z1]
max.y.size = [s0, 1]  # Perform a reduce operation on the z1 axis
max.y.strides = [1, 0]
```

abs is an elewise operator, and its axes do not differ in computation. Therefore, as long as the memory is contiguous, multiple axes can be merged into a single axis for tiling. For example, as shown in [Figure 2](#fig2), block tiling is first performed on the axis, and 15 is divided into three blocks, with block0 being the purple part. Tiling is then performed within block0. At this point, the tiling block does not fully occupy the portion allocated to block0, so a for loop needs to be added within the block.

**Figure 2**  Elewise operator partitioning<a id="fig2"></a>
![Figure 2](../figures/elewise_op_split.png "Elewise operator partitioning")

Splitting of a reduce operator is more complex and requires dual tiling in implementation: the row direction is the elewise axis, and the column direction is the reduce axis. First, blocks are split in the row direction, with a loop written inside each block, and then another for loop is added in the column direction.

**Figure 3**  Reduce operator partitioning
![Figure 3](../figures/reduce_op_split.png "Reduce operator partitioning")

Merging rules of TilingGroup:

```text
()(z0,z1)() Merge ()(z0)(z1) => ()(z0)(z1)
```

The \(z0 and z1\) axes that originally belonged to the ygroup of the abs operator are split, and the z1 axis is adjusted to the rgroup. The core purpose of this adjustment is to keep the abs operator consistent with the subsequent reduce-type operator in terms of the tiling strategy.

## Parallel Optimization

- Loop Splitting

    The core purpose of loop tiling is to introduce new loop levels, clearly separating the outer loops suitable for parallelism from the inner loops suitable for vectorization.

    For each TilingCase, the axes in xgroup, ygroup, and rgroup are split into ub\_out and ub\_in:

    ![fig](../figures/auto_quantization_flow.png)

    For example: \{z0, z1, z2\}, when tiling is performed on z1, z1 is split into z1T and z1t, where z1T is ub\_out and z1t is ub\_in.

- Vectorization

    Vectorization is a key technique that leverages the hardware SIMD (Single-Instruction Multiple-Data stream processing) unit to improve the efficiency of data-parallel computation. By converting single-element operations into vector operations, it significantly reduces the number of instruction executions and improves hardware utilization.

    For example, for the following loop:

    ```c++
    for (int i = 0; i < 256; i++) {
        c[i] = a[i] + b[i];
    }
    ```

    Non-vectorized execution requires 256 addition instructions, while vectorized execution requires only one addition instruction.

    After an axis is selected within each group as the UB tiling axis, ub\_in and its inner axes are used as the vectorization axes. In this case, because the groups are generated in the xyr order, the vectorization axes generated in this order have an axis order inconsistent with the memory layout, which introduces non-contiguous data movement. Therefore, the axes need to be reordered according to the output axis order.

    ![fig](../figures/auto_quantization_flow_8.png)

    For example, if the output tensor axis order is \(a, b, c, d\), and the axis groups are \(a, c\),\(b, d\)\(\), the vectorization axis groups generated in this order are \(a\_in, c, b\_in, d\), which need to be adjusted to \(a\_in, b\_in, c, d\).

    > [!NOTE]
    >
    > In the Schedule phase, the same vectorization axes are set uniformly for the entire graph. For some APIs, due to instruction and other limitations, not all vectorization axes can be vectorized. In this case, the CodeGen phase needs to unroll the axes that cannot be vectorized into for loops.
    > For example, if the vectorization axes are \[z1,z2,z3\], which is equivalent to three nested loops, and the instruction supports three nested loops, it can be written as vector\[z1,z2,z3\]. However, if the instruction supports only two nested loops, CodeGen needs to generate the following code:
    >
    >```text
    >for (i in z1) {
    > vector[z2,z3]
    >}
    >```

- Loop merging and loop core binding

    Loop merging and loop core binding are often used together for optimization. In the loop tiling phase, an axis of ub\_out is generated within each group.

    1. Merge all non-reduce axes into one axis and all reduce axes into one axis to reduce the number of loop nesting levels.
    2. Split the merged loops to obtain the outer loop and the inner loop.
    3. Bind the split outer loop to multiple blocks to achieve parallelism, and consume the inner loop through iteration.

    ![fig](../figures/auto_quantization_flow_9.png)

    For example: \{z0, z1T, z1t, z2\}, z0 and z1T are first merged into z0z1T, and then multi-core tiling is performed on z0z1T. Since z0z1T may exceed the number of logical AI Cores participating in the computation, an additional loop layer is required after core allocation, and z0z1T is further split into z0z1TB and z0z1Tb. The specific values are calculated by Auto Tiling during the tiling phase.

## Memory Optimization

Currently, memory reuse is mainly performed based on node reference relationships. To improve the reuse effect, memories of similar sizes are allocated to the same group as much as possible, and then reused within the group.

The pseudocode for memory reuse is as follows:

```c++
for (node in all nodes) {
  for (output in node.outputs) {
    // Mark the number of dependencies of the tensor.
    output->sch.depends = output->anchor->GetPeerInDataNodesSize();
    // try reuse from free queue
  }
  for (input in node.inputs) {
    input->sch.depends--;
    if (input->sch.depends == 0) {
        Enqueue(input->opt.reuse_id); // Mark it as freeTensor so that it can be reused by subsequent nodes.
    }
  }
}
```

For some APIs, the output can directly reuse the input. For such APIs, inplace reuse can be adopted, that is, the output directly reuses the input memory. As shown in the following figure, three memory blocks are required before inplace reuse:

**Figure 4** Before memory reuse
![Figure 4](../figures/memory_reuse_before.png "Before memory reuse")

After inplace reuse, only two memory blocks are required:

**Figure 5** After memory reuse
![Figure 5](../figures/memory_reuse_after.png "After memory reuse")

## Multi-template Generation

For a computation graph, there may be multiple implementation approaches. Taking tail-axis concat as an example, you can first combine multiple small packets into a large packet on the UB and then move it out completely, or you can directly convert it into non-contiguous movement and complete the rearrangement on the GM (Global Memory). The former can significantly improve the MTE (Memory Transfer Engine, the data transfer engine of the AI Core) transfer efficiency in small-shape scenarios, thereby achieving better performance advantages. However, it also has the limitation of requiring the inner axis to be fully loaded, which makes it unusable in certain scenarios. When it is impossible to determine which template to select during the Schedule phase, a general template applicable to any shape is usually generated, along with performance-optimized templates for specific scenarios, and the Auto Tiling module decides which template to use during the tiling phase.

- UB concat template:

    ![fig](../figures/fake_quant_model.png)

- Graph modification template:

    ![fig](../figures/fake_quant_model_10.png)
