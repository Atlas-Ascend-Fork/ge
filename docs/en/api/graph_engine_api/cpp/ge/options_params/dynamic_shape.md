# Dynamic Shape

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:38:50.578Z pushedAt=2026-10-28T07:42:19.742Z nl=2 -->

## ac\_parallel\_enable

Whether AI CPU operators and AI Core operators are allowed to run in parallel in a dynamic shape graph.

In a dynamic shape graph, when this switch is enabled, the system automatically identifies AI CPU operators that can run concurrently with AI Core operators, and dispatches operators from different engines to different streams to achieve parallelism across multiple engines, thereby improving resource utilization and dynamic shape execution performance.

**Parameter values:**

- 1: Allows parallel execution between AI CPU and AI Core operators.
- 0: (Default value) AI CPU operators are not dispatched to a separate stream.

**Configuration example:**

```c++
{"ac_parallel_enable", "1"};
```

**Required/Optional**: Optional

**Effective level**: Global

## ge.dynamicDims

Sets the gears of dynamic dimensions in ND format. It applies to scenarios where dimensions of any size are processed during each inference. This parameter must be used in conjunction with ge.inputShape. For detailed usage examples, see [Compiling and Running Graph Scenarios](../../../../../user_guides/graph_dev/more_features/dynamic_shape.md#graph-build-and-run).

**Parameter values:**
Set the value in the format of "dim1,dim2,dim3;dim4,dim5,dim6;dim7,dim8,dim9".

**Parameter value format:** All gears must be enclosed in double quotation marks. Gears are separated by English **semicolons**, and dimensions within each gear are separated by English **commas**. The dim values in each gear correspond in sequence to the parameters identified by -1 in the ge.inputShape parameter. The number of dimensions in each gear must equal the number of -1 values in the inputShape parameter. In addition, the number of gears must be greater than 1.

<!-- npu="950,A3,910b,910,310p" id7 -->
**Parameter value constraints:**
<!-- end id7 -->

<!-- npu="950" id1 -->
- For Ascend 950PR&950DT products, the number of gears ranges from \(1, 256\], that is, at least 2 gears must be set, and a maximum of 256 gears are supported. It is recommended to configure 3\~4 gears.
<!-- end id1 -->

<!-- npu="A3,910b,910,310p" id2 -->
- For the following products, the number of gears ranges from \(1,100\], that is, at least 2 gears must be set, and a maximum of 100 gears are supported. It is recommended to configure 3\~4 gears.

    <!-- npu="A3" id3 -->
    Atlas A3 products
    <!-- end id3 -->

    <!-- npu="910b" id4 -->
    Atlas A2 products
    <!-- end id4 -->

    <!-- npu="910" id5 -->
    Atlas training products
    <!-- end id5 -->

    <!-- npu="310p" id6 -->
    Atlas inference products
    <!-- end id6 -->
<!-- end id2 -->

Usage example:

- If the model has only one input:

    ```c++
    {"ge.inputShape", "data:1,-1,-1"},
    {"ge.dynamicDims", "1,2;3,4;5,6;7,8"}
    // During graph runtime, the supported shapes of the data operator are 1,1,2; 1,3,4; 1,5,6; 1,7,8
    ```

- If the network model has multiple inputs:

    The dim values in each gear correspond in sequence to the parameters identified by -1 in the model input parameters. The number of dimensions that must be set in each gear equals the number of -1 occurrences in the model input parameters. For example, a network model has three inputs, namely data\(1,1,40,T\), label\(1,T\), and mask\(T,T\), where T is dynamically variable. The configuration example is as follows:

    ```c++
    {"ge.inputShape", "data:1,1,40,-1;label:1,-1;mask:-1,-1" },
    {"ge.dynamicDims", "20,20,1,1;40,40,2,2;80,60,4,4"}
    // During graph runtime, the supported input dims combination gears are as follows:
    // Gear 0: data(1,1,40,20)+label(1,20)+mask(1,1)
    // Gear 1: data(1,1,40,40)+label(1,40)+mask(2,2)
    // Gear 2: data(1,1,40,80)+label(1,60)+mask(4,4)
    ```

**Required/Optional**: Optional

**Effective level**: session/graph

## ge.dynamicNodeType

Specifies the node type for dynamic input.

- 0: The dataset input is a dynamic input.
- 1: The placeholder input is a dynamic input.

Currently, dataset and placeholder inputs cannot both be dynamic inputs.

**Configuration example:**

```c++
{"ge.dynamicNodeType", "0"};
```

**Required/Optional**: Optional

**Effective level**: session/graph

## ge.exec.hostSchedulingMaxThreshold

Maximum threshold that controls whether dynamic shape scheduling is used when a static small graph (root graph) is executed. The default threshold is 0. It is recommended for large model scenarios.

- If the number of static root graph nodes is less than the maximum threshold, dynamic shape scheduling is used. In large model scenarios, this approach saves stream resources.
- If the number of static root graph nodes \> the maximum threshold, the original process remains unchanged.

Note: If a static root graph node includes a subgraph, this parameter does not take effect.

**Required/Optional**: Optional

**Effective level**: Global/session/graph

## ge.exec.static\_model\_ops\_lower\_limit

In dynamic shape splitting, sets the minimum operator count threshold for a **static subgraph**.

**Parameter values:**

The value ranges from -1 to positive infinity. Any other value causes an error. The default value is 4.

- Value \>=0:
  - If the operator count is less than the threshold, the static subgraph is not split separately; instead, it is merged into the dynamic graph for dynamic execution.
  - If the operator count is \>= the threshold, the original process remains unchanged (the static subgraph is still split).

- Value = -1: All operators are executed through the dynamic graph.

For example, if the operator count in a static subgraph is 4 and the threshold configured through this parameter is 10, the static subgraph is not split separately and is executed through the dynamic graph.

**Required/Optional**: Optional

**Effective level**: graph

## ge.inputShape

Specifies the shape of the model input data. In online inference or training scenarios, this parameter is a mandatory configuration only in the **dynamic bucketing scenario** (dynamic dimensions). In non-dynamic bucketing scenarios, configuring this option alone does not take effect.

In the dynamic bucketing scenario, one or more dimension values of the input data in the original model are not fixed. Currently, the model can be converted by configuring shape bucketing.

**Parameter values:**

- The model has a non-fixed shape

    When configuring the ge.inputShape parameter, set the corresponding dimension value to -1. This parameter must be used in conjunction with ge.dynamicDims (which sets the dynamic dimension gears) and ge.dynamicNodeType (which specifies the node type of the dynamic input).

- The model shape is scalar + dynamic bucketing

    If the model input contains both scalar shapes and shapes that support dynamic bucketing, the scalar inputs cannot be ignored and must be configured. For example, if a model has three inputs, namely A:\[-1,c1,h1,w1\], B:\[\], and C:\[n2,c2,h2,w2\], the shape information is "A:-1,c1,h1,w1;**B:**;C:n2,c2,h2,w2", and the scalar input B must be configured.

**Configuration example:**

- Non-fixed shape:

    For an example of setting a gear for a specified dimension, see ge.dynamicDims.

- shape is a scalar + dynamic bucketing

    Mandatory configuration for inputs whose shape is a scalar. For example, if a model has three inputs with shapes A:\[-1,32,208,208\], **B:\[\]**, and C:\[16,64,208,208\], the configuration example is as follows (A is a dynamic bucketing input; here, setting a dynamic dimension gear is used as an example):

    ```c++
    {"ge.inputShape", "A:-1,32,208,208;B:;C:16,64,208,208"},
    {"ge.dynamicDims", "1;2;4"}
    ```

**Required/Optional**: Optional

**Effective level**: session/graph
