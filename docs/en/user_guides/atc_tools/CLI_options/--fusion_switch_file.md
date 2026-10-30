# --fusion\_switch\_file

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:05:54.545Z pushedAt=2026-10-27T09:16:37.547Z -->

## Product Support

All chips are supported.

## Feature Overview

Specifies the path and file name of the configuration file for fusion rules (including graph fusion and UB fusion) switches. This parameter **disables** the fusion rules specified in the configuration file.

- Graph fusion: a process in which the FE modifies the graph based on fusion rules. Graph fusion replaces the pre-fusion operators in the graph with the fused operator to improve computing efficiency. Graph fusion applies to the following scenarios:
  - When the mathematical computation of certain operators can be optimized, graph fusion can be performed to save computing time. For example, conv+biasAdd can be fused into a single operator that directly completes the accumulation in L0C, thereby eliminating the add computation process.
  - When the fused computation process can be accelerated by hardware instructions, graph fusion can be performed to achieve acceleration. For example, the accumulation process of conv+biasAdd is accelerated by the accumulation function in L0C and can be completed through graph fusion.

- UB fusion: UB refers to the Unified Buffer on the AI processor. UB fusion means that the computation result of operator A resides in the Unified Buffer and needs to be moved to Global Memory. When operator B is executed, the output of operator A needs to be moved from Global Memory back to the Unified Buffer for the computation logic of B. After the computation is complete, the result is moved from the Unified Buffer back to Global Memory.

    From this process, it can be seen that the result of A is moved from Unified Buffer-\>Global Memory-\>Unified Buffer-\>Global Memory. This data movement process through Global Memory is wasteful. Therefore, operators A and B are merged into a single operator to eliminate the data movement process, which is called UB fusion. UB fusion can reduce the data movement time across the entire network (Global Memory\>Unified Buffer, Unified Buffer-\>Global Memory), improve computing efficiency, and effectively reduce bandwidth.

    <!-- npu="950" id1 -->
    The Ascend 950PR&950DT products do not support UB fusion.
    <!-- end id1 -->

## Related Parameters

None.

## Parameter Values

**Parameter value:**
Path and file name of the configuration file.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter value constraints:**

The built-in graph fusion and UB fusion rules are all enabled by default. You can use this parameter to disable specified fusion rules as needed. For the fusion rules that can be disabled, see *Graph Fusion and UB Fusion Rules Reference*. Due to system mechanisms, other fusion rules cannot be disabled.

## Recommended Configuration and Benefits

None.

## Example

- **Scenario 1: Configure the fusion rules to be disabled one by one**

    A sample configuration file is as follows. The text before the colon is the fusion rule name, and the field after it indicates whether the fusion rule is enabled (an example of the fusion rule switch configuration file name is fusion\_switch.cfg):

    ```text
    xxxFusionPass:off
    yyyFusionPass:off
    ....
    ```

- **Scenario 2: Disable fusion rules with one click**

    This parameter allows users to disable fusion rules with one click.

    ```json
    {
        "Switch":{
            "GraphFusion":{
                "ALL":"off"
            },
            "UBFusion":{
                "ALL":"off"
                }
        }
    }
    ```
  <!-- npu="950" id2 -->
  **When UB fusion is not supported, UBFusion can be deleted from the preceding configuration file.**
  <!-- end id2 -->

    Note:

    1. Disabling certain fusion rules may cause functional issues. Therefore, the one-click disabling here disables only some of the system fusion rules, not all of them.
    2. When disabling fusion rules with one click, you can enable some fusion rules at the same time:

        Sample configuration file:

        ```json
        {
            "Switch":{
                "GraphFusion":{
                    "ALL":"off",
                    "SoftmaxFusionPass":"on"
                },
                "UBFusion":{
                    "ALL":"off",
                    "TbePool2dQuantFusionPass":"on"
                }
            }
        }
        ```

        <!-- npu="950" id3 -->
        **When UB fusion is not supported, UBFusion can be deleted from the preceding configuration file.**
        <!-- end id3 -->

Upload the configured _fusion\_switch.cfg_ file to any directory on the server where the ATC tool is located, for example, to $HOME/module. The usage example is as follows:

```bash
atc --fusion_switch_file=$HOME/module/fusion_switch.cfg ...
```

After model conversion is complete, whether an operator fusion information result file "fusion\_result.json" is generated depends on the value of the [--export\_compile\_stat](--export_compile_stat.md) parameter.

This file records the fusion rules that are still used during graph compilation, excluding the fusion rules disabled in the fusion\_switch.cfg file. The "match\_times" field indicates the number of times a fusion rule is matched during model conversion, and the "effect\_times" field indicates the number of times it actually takes effect. If the [--fusion\_switch\_file](--fusion_switch_file.md) parameter is not configured, the generated "fusion\_result.json" file records all fusion rules matched during model conversion.

## Constraints

- If the value of the "group" attribute of the Convolution operator in the network model is equal to the value of the “num\_output” attribute in the model file prototxt, **VxxxRequantFusionPass** in the preceding configuration file must be enabled.

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--fusion_switch_file_res.md#id1 -->

<!-- npu="A3,910b,910,310p,310b" id8 -->
- When AMCT quantizes the original framework model, it inserts quantization and dequantization operators. During model conversion using the ATC tool, the inserted quantization and dequantization operators are fused. In this case, comparing the dump result of the quantized model with that of the original model may be inaccurate. Therefore, if you want to use the AMCT-quantized model for accuracy comparison, you need to disable some fusion functions through the [--fusion\_switch\_file](--fusion_switch_file.md) parameter. The fusion rules that must be disabled in this scenario are as follows:

    <!-- npu="910" id4 -->
    Fusion rules that must be disabled in the Atlas training products scenario:

    ```text
    V100RequantFusionPass:off
    ConvConcatFusionPass:off
    SplitConvConcatFusionPass:off
    TbeEltwiseQuantFusionPass:off
    TbeConvDequantVaddReluQuantFusionPass:off
    TbeConvDequantVaddReluFusionPass:off
    TbeConvDequantQuantFusionPass:off
    TbeDepthwiseConvDequantFusionPass:off
    TbeFullyconnectionElemwiseDequantFusionPass:off
    TbeConv2DAddMulQuantPass:off
    TbePool2dQuantFusionPass:off
    TbeCommonRules0FusionPass:off
    TbeCommonRules2FusionPass:off
    ```
    <!-- end id4 -->

    <!-- npu="310p" id5 -->
    Fusion rules that must be disabled for Atlas inference products:

    ```text
    V200RequantFusionPass:off
    ConvConcatFusionPass:off
    SplitConvConcatFusionPass:off
    TbeEltwiseQuantFusionPass:off
    TbeConvDequantVaddReluQuantFusionPass:off
    TbeConvDequantVaddReluFusionPass:off
    TbeConvDequantQuantFusionPass:off
    TbeDepthwiseConvDequantFusionPass:off
    TbeFullyconnectionElemwiseDequantFusionPass:off
    TbeConv2DAddMulQuantPass:off
    TbePool2dQuantFusionPass:off
    TbeCommonRules0FusionPass:off
    TbeCommonRules2FusionPass:off
    ```
    <!-- end id5 -->

    <!-- npu="910b,310b" id6 -->
    Fusion rules that must be disabled for Atlas 200I/500 A2 inference product and Atlas A2 products:

    ```text
    ConvConcatFusionPass:off
    SplitConvConcatFusionPass:off
    TbeEltwiseQuantFusionPass:off
    TbeConvDequantVaddReluQuantFusionPass:off
    TbeConvDequantVaddReluFusionPass:off
    TbeConvDequantQuantFusionPass:off
    TbeDepthwiseConvDequantFusionPass:off
    TbeFullyconnectionElemwiseDequantFusionPass:off
    TbeConv2DAddMulQuantPass:off
    TbePool2dQuantFusionPass:off
    TbeCommonRules0FusionPass:off
    TbeCommonRules2FusionPass:off
    ```
    <!-- end id6 -->

    <!-- npu="A3" id7 -->
    Fusion rules that must be disabled for Atlas A3 products:

    ```text
    ConvConcatFusionPass:off
    SplitConvConcatFusionPass:off
    TbeEltwiseQuantFusionPass:off
    TbeConvDequantVaddReluQuantFusionPass:off
    TbeConvDequantVaddReluFusionPass:off
    TbeConvDequantQuantFusionPass:off
    TbeDepthwiseConvDequantFusionPass:off
    TbeFullyconnectionElemwiseDequantFusionPass:off
    TbeConv2DAddMulQuantPass:off
    TbePool2dQuantFusionPass:off
    TbeCommonRules0FusionPass:off
    TbeCommonRules2FusionPass:off
    ```
    <!-- end id7 -->

    The fusion rules are briefly described as follows. For details, see *Graph Fusion and UB Fusion Rule Reference*.

  <!-- npu="910,310p" id10 -->
  - V100RequantFusionPass

    Graph fusion rule. Inserts the RequantHostCpuOpV2 operator at the input of AscendDequant.

    This fusion rule is supported only on Atlas training products and Atlas inference products.
  <!-- end id10 -->

  <!-- npu="310p" id9 -->
  - V200RequantFusionPass

    Graph fusion rule that fuses AscendDequant and AscendQuant into AscendRequant and inserts the RequantHostCpuOpV2Re operator at the input of AscendDequant.

    This fusion rule is supported only on Atlas inference products.
  <!-- end id9 -->

  - ConvConcatFusionPass

    Graph fusion rule that supports the graph fusion rule of the conv2d\*N+concat operator. The conv2d operator can be followed by dequant and Relu-type operators.

  - SplitConvConcatFusionPass

    Graph fusion rule that supports the fusion rule of the split+conv2d\*N+concat operator. The conv2d operator can be followed by dequant and Relu-type operators.

  - TbeEltwiseQuantFusionPass

    UB fusion rule that supports UB fusion of elemwise+quant operators, where the quant operator is an optional node.

  - TbeConvDequantVaddReluQuantFusionPass

    UB fusion rule. In a quantization scenario, marks consecutive Conv-dequant-vadd-relu-quant nodes for UB fusion to improve inference performance.

  - TbeConvDequantVaddReluFusionPass

    UB fusion rule. Supports the fusion node of the conv2d+dequant+vadd+relu/conv2d+dequant+\(leakyrelu\)+vadd operator.

  - TbeConvDequantQuantFusionPass

    UB fusion rule. In a quantization scenario, marks consecutive Conv-dequant-quant nodes for UB fusion to improve inference performance.

  - TbeDepthwiseConvDequantFusionPass

    UB fusion rule. Supports the fusion node of the depthwiseConv2d+dequant+\(relu/mul\)+quant/depthwiseConv2d+dequant+\(sigmoid\)+mul/depthwiseConv2d+requant/depthwiseConv2d+\(power+relu6+power\)+elemwise+\(quant\) operator.

  - TbeFullyconnectionElemwiseDequantFusionPass

    UB fusion rule. Supports the following two fusion forms:

    1. Fusion of BatchMatMul/BatchMatMulV2 + elemwise in static shape scenarios.
    2. Fusion of MatMul/MatMulV2/BatchMatMul/BatchMatMulV2 + AscendDequant + elemwise1\(+ elemwise2\) in static shape scenarios.

  - TbeConv2DAddMulQuantPass

    UB fusion rule. It supports conv+dequant+add+quant fusion. The add operator can be fused only when it has two other arbitrary outputs in addition to quant.

  - TbePool2dQuantFusionPass

    UB fusion rule. In quantization scenarios, it marks consecutive Pool2d-quant nodes for UB fusion to improve inference performance.

  - TbeCommonRules0FusionPass

    UB fusion rule. It supports UB fusion of the StridedRead+Conv2D+dequant+elemwise+quant+StridedWrite operators. Except for Conv2D, all other nodes are optional.

  - TbeCommonRules2FusionPass

    UB fusion rule. It supports UB fusion of the StridedRead+Conv2D+dequant+elemwise+quant+StridedWrite operators. Except for Conv2D, all other nodes are optional. elemwise supports fusion in multi-output scenarios.

  <!-- end id8 -->
