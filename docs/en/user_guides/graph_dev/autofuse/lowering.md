# Lowering

<!-- md-trans-meta sourceCommit=cc5ba9391389e272005d4ade3dda4ccce784361d translatedAt=2026-10-21T09:03:57.645Z pushedAt=2026-10-28T03:30:34.450Z nl=2 -->

## Overview

The Lowering module is responsible for implementing operator fusion based on loop-axis merging on the GE graph. The input of this module is the GE graph after symbolic derivation, and the output is a GE graph containing fused operators. Fused operators are expressed using operators of the AscBackend type. An AscBackend operator holds an attribute of the AscGraph type to express its computation logic. After the lowering phase, modules such as CanFuse, Schedule, and Codegen further perform secondary fusion between AscBackend nodes, code generation, and compilation.

The Lowering module consists of two levels: operator-level lowering and graph-level lowering (graph lowering).

- Operator-level lowering uses low-level IR to express the computation logic of operators as Scalar expressions. A Scalar expression describes how the value of each output is computed. Operators that can be expressed as Scalar expressions fall into three types:
  - Pointwise type

    That is, the output shape is the result of broadcasting multiple inputs, and the value of each output is computed from the values of the inputs at the corresponding positions. Most common GE IRs are of this type, such as Add, Mul, Exp, and Abs.

  - Reduction type

    Each element of the output is obtained by reducing multiple input elements. Examples include ReduceSum and ReduceMax.

  - View type

    Each element of the output equals an element at a specific position in the input, with no computation in between. The number of output elements can differ from the number of input elements. For example, the Broadcast operator can map multiple output elements to the same input element. The View types currently supported by lowering include Broadcast and others.

- Graph-level lowering invokes the lowering implementation of each operator on the graph in sequence, and performs operator fusion based on Scalar expressions. Graph-level lowering completes fusion in the following scenarios:
  - Pointwise + Pointwise
  - Pointwise + View
  - Pointwise -\> Reduction

## Operator Lowering Implementation

Operator lowering is the use of Loop IR to express the computation logic of an operator. Taking the basic Add node as an example, its computation process mainly includes:

1. Load data from the input Tensor.
2. Broadcast the input data.
3. Determine the dtype used for computation, and convert the inputs to the corresponding type.
4. Perform the Add computation on the loaded data.
5. Write the result to the output Tensor of the node.

When compiling a GE graph, node Anchors are used to express the runtime Tensor inputs. An Anchor can be understood as a compile-time placeholder for a Tensor input. The following is a pseudocode example of using Loop IR to express the Add computation:

```c++
graphStatus LoweringAdd(const ge::Node &node) {
    auto x = loop::Load(node.GetInDataAnchor(0)); // Load data from input anchor 0
    auto y = loop::Load(node.GetInDataAnchor(1)); // Load data from input anchor 1
    vector<Expression> broadcasted_shape = xxx; // Compute the broadcast output.
    ge::DataType compute_dtype = xxx; // Compute the compute dtype after type promotion. Note that this step differs from the proactive type promotion during Codegen.
    x = loop::Broadcast(x, broadcasted_shape);
    x = loop::Cast(x, compute_dtype);
    y = loop::Broadcast(y, broadcasted_shape);
    y = loop::Cast(y, compute_dtype);
    auto result = loop::Add(x, y); // Express the computation.
    loop::Store(node.GetOutDataAnchor(0), result); // Save the computation result to the output anchor.
}
```

Because operations such as loading inputs, saving outputs, computing broadcast, and type promotion are highly similar, after extracting the common implementation, the lowering of an operator can be simplified to implementing only the computation part:

```c++
Var LoopAdd(const vector<LoopVar> &inputs) { // Var is the intermediate result type of the loop computation output.
     return loop::Add(inputs[0], inputs[1]);
 }
```

Finally, LoopAdd is called during the lowering process:

```c++
graphStatus LoweringAdd(const ge::Node &node) {
     // x, y = common logic for loading, broadcasting, and type conversion of inputs
     auto result = LoopAdd(x, y); // Express the computation.
     // Common logic for saving the output
 }
```

## Graph Lowering Implementation

Graph lowering traverses the operators on the graph and calls the lowering function of each operator in turn. If an operator does not have a lowering function, the default lowering function is executed.

An example of the operator lowering function prototype is as follows: it takes an operator node on the GE graph as input, returns whether the lowering succeeds, and internally completes the Scalar expression of the node, storing the result in an attribute.

```c++
ge::graphStatus(const NodePtr &node);
```

Graph lowering is divided into three steps as a whole:

1. Call the lowering function of each node in turn to obtain its corresponding Loop expression.
2. Derive the index and range on the Loop expression, so as to express the View class in the intermediate process in terms of the data movement method of the input.
3. Combine the index and range information derived from the Loop expression to generate a set of axis-based AscGraph.

After the node lowering function is called, the node is not immediately converted to the AscBackend fusion node type. Instead, each output Tensor of each node is associated with a KernelBox object. KernelBox is used to express the computation process of an operator output Tensor. Its core definition is as follows:

```c++
class KernelBox {
  public:
   bool IsExternKernel() const; // Whether it is of the Extern type (that is, the original GE IR implementation is used). The value true indicates that the original GE IR implementation is used.
    // Handle the behavior when loading an element from this KernelBox. For an Extern or Realized KernelBox, it is equivalent to directly loading data from its output anchor. For a KernelBox that can continue to be fused, its internal computation process is returned.
   LoopVar Load() const;
   // Perform Realize, that is, generate an AscBackend node for this KernelBox. If true, it indicates that fusion is possible.
   void Realize(bool persistent = true);
 };
```

### Single-Output Multi-Reference

For a single-output multi-reference operator, multiple computations may load its output in inconsistent ways. Graphlowering performs lowering based on recomputation. That is, for a node that can continue to be fused, if its output is used by multiple nodes, such as the first Abs in the following example, the computation of the first Abs is included in the KernelBox corresponding to each node that uses the output. Through recomputation, the scenario where the same data is moved in different ways can be handled.

CSE (Common Subexpression Elimination) in the figure is an optimization technique commonly used in compiler optimization, code generation, and mathematical computation. Its core objective is to identify and reuse repeated subexpressions in an expression, thereby reducing redundant computation and improving efficiency.

**Figure 1** Single-output multi-reference process
![Figure 1](../figures/single_output_multi_ref_flow.png "Single-output multi-reference process")

### Fusion and Fusion Termination Strategies

As for whether to perform fusion and when to terminate fusion during lowering, this section presents the current fusion scope control strategies:

1. The type of the KernelBox is the Reduction type.
2. The total number of Loop nodes in the KernelBox exceeds the threshold, and the current threshold is 64.
3. The number of Load nodes in the KernelBox exceeds the threshold, and the current threshold is 4.
4. The KernelBox serves as the input of a node that cannot be lowered (triggered in the default lowering function).
5. The node to which the KernelBox belongs contains input or output control edges (fusing across nodes loses accurate control information).
6. The stream label information of the node to which the KernelBox belongs is inconsistent with the stream label of the node that uses the KernelBox as input. (If either side is not labeled, they are considered consistent.)
7. When an Exp operator is encountered, it is immediately realized.
8. Operators in different kernels of the kernel control scope are not fused into the same fusion operator.
9. Unused inputs are realized in advance. For example, Zeroslike.
10. If the operator has the \_super\_kernel\_scope attribute, it is skipped directly.
11. Operators with \_disable\_autofuse\_scope skip lowering.
12. The actual attributes are inconsistent with the operator IR.
13. If the dtype check of the operator is not supported, it is skipped.
14. A pure scalar graph skips fusion (limited to a single AscBackend).

For operators that do not support lowering, the default lowering implementation is called. The default lowering function performs two actions:

1. Realize the KernelBox corresponding to all input Anchors to terminate fusion.
2. Set the KernelBox corresponding to the node's own output anchor to a KernelBox of the ExternKernel type.

Currently, there are three scenarios that use the default lowering implementation, and they are handled in exactly the same way:

- The node cannot be expressed through lowering.
- The node can be expressed through lowering, but the implementation is not provided.
- The node can be expressed through lowering and the implementation is provided, but the expression result is invalid because no symbolic derivation result is available.

When writing a lowering implementation for a node, there is no need to consider whether a symbolic result exists, because this is determined in the Loop function. Regardless of the specific situation, the final KernelBox after lowering is consistent, and is always a KernelBox of the ExternKernel type.

### Fusion Rollback Policy

Rollback refers to how an AscBackend fusion node should be rolled back to the original node for execution when, during the lowering phase, the generated AscBackend fusion node performs worse than the original node because the specific CanFuse/Schedule/Codegen capabilities are not perceived. The current rollback policies include:

If the number of original GE IR nodes corresponding to the AscBackend fusion node is less than the threshold (the current threshold is 2), the node is rolled back to the original node for execution.

## Lowering Process Example

This section uses a simple GE IR graph of Conv+Abs+Exp+Conv (Conv has no lowering implementation) as an example to briefly introduce the lowering process. The original GE IR graph is as follows:

**Figure 1** Original GE IR graph
![Figure 1](../figures/original_ge_ir_graph.png "Original GE IR graph")

1. First, lowering is performed on Conv. Since Conv has no registered lowering implementation, the default lowering is executed: the KernelBox of all output anchors is of the ExternKernel type, indicating that the original GE IR Kernel is executed.

    **Figure 2** Lowering of the input Conv
    ![Figure 2](../figures/input_conv_lowering.png "Lowering of the input Conv")

2. Then, during lowering of Abs, the input data is loaded. The pseudocode is as follows:

    ```c++
    graphStatus LoweringAbs(const ge::Node &node) {
         auto x = loop::Load(node.GetInDataAnchor(0)); // Load data from input anchor 0
         ...
         auto result = loop::Abs(x);
         ...
     }
    ```

    When loop::Load\(node.GetInDataAnchor\(0\)\) is executed, the Load method of the KernelBox associated with the peer output anchor is actually called. At this point, it is the output of the Conv node. Since the KernelBox of Conv is of the Extern type, loading from it is equivalent to loading from the output anchor of the Conv node. Therefore, after the Abs node is lowered, the KernelBox associated with its output anchor is as follows:

    **Figure 3** Lowering of Abs
    ![Figure 3](../figures/abs_lowering.png "Lowering of Abs")

3. Next, lowering is performed on the Exp node. The pseudocode is as follows:

    ```c++
    graphStatus LoweringExp(const ge::Node &node) {
         auto x = loop::Load(node.GetInDataAnchor(0)); // Load data from input anchor 0
         ...
         auto result = loop::Exp(x);
         ...
     }
    ```

    When loading its input, what is actually executed is the Load of the KernelBox associated with the output anchor of the Abs node. Since both **IsRealize** and **IsExternKernel** are **false** for this node (if **IsRealize** is **true**, it can be fused; if **IsExternKernel** is true, it follows the original GE IR process), fusion can continue, and therefore its internal computation process (load-\>Abs-\>Store) is returned. After lowering of Exp, the KernelBox associated with its output anchor is as follows:

    **Figure 4**  Lowering of Exp
    ![Figure 4](../figures/exp_lowering.png "Lowering of Exp")

4. Finally, the lowering of the Conv node is performed, which executes the default lowering function. It realizes the KernelBoxes on the opposite ends of all input Anchors (because Conv needs to fall back to the original GE IR Kernel for execution, which requires inputs).

    **Figure 5**  Lowering of the output Conv
    ![Figure 5](../figures/output_conv_lowering.png "Lowering of the output Conv")

After the graph lowering is completed, it can be seen that the KernelBox that needs to be realized is the KernelBox output by the Exp node. At this point, the KernelBox associated with the output anchor of the Exp node is converted into an AscGraph, and the Exp node is replaced with the AscBackend type. Note that the input of the KernelBox associated with the Exp node is Conv:0 rather than the Abs node. Therefore, after the replacement, the Abs node is deleted because its output is no longer used by any node. However, to ensure that the original nodes can be rolled back conveniently at any time, the deletion of unused nodes is not executed immediately.
