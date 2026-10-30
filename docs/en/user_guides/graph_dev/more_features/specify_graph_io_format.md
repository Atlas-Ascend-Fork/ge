# Specifying the Internal Formats of Graph Inputs and Outputs

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-21T10:02:05.803Z pushedAt=2026-10-28T08:41:06.311Z nl=2 -->

## Feature Overview

Before learning about the features in this section, you need to understand what raw format and internal format (also called runtime format) are:

The raw format refers to the original image format that has not undergone any conversion, corresponding to the format used in scripts, such as NCHW and NHWC. To ensure that operators deliver the highest efficiency on different hardware specifications, the internal format (corresponding to the format used for computation on the Device) is introduced, with common ones including NC1HWC0, FRACTAL\_NZ, FRACTAL\_Z, and so on. For example, if a Tensor in the NCHW format is used in a script developer's model, after optimization by the graph compilation framework, the Tensor may be converted to the runtime format NC1HWC0. For details about formats, see "Programming Guide \> Concepts, Principles, and Terms \> Neural Networks and Operators \> Data Layout Format" in [Ascend C Operator Development](https://gitcode.com/cann/asc-devkit/blob/9.2.0/docs/zh/guide/index.md).

Converting the raw format to the runtime format is an extra overhead relative to the original script. Therefore, the feature described in this section is provided to allow users to specify the internal format of model inputs and outputs, thereby reducing the extra format conversion overhead incurred when Tensors are transferred across graph boundaries and achieving greater performance gains.

The main APIs involved in this feature are as follows:

![Illustration](../figures/feature_intro_12.png)

After creating a Graph instance, call the SetStorageFormat API on the output TensorDesc of a Data node or on the TensorDesc of a model output to set the runtime format, call the SetExpandDimsRule API to set the dimension expansion rule, and then set the input and output operators in the Graph to complete Graph construction.

For details about the SetStorageFormat and SetExpandDimsRule APIs, see [Basic Data Structures and APIs](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md).

**Usage constraints:**

- To set the internal format of Graph inputs, currently only Data nodes and RefData nodes are supported. Other types of nodes are not supported yet.
- This feature supports only the [Building and Running a Graph](../compile_and_run_graph/compile_and_run_graph.md) scenario, and does not support the [Building a Graph into an Offline Model](../compile_and_run_graph/compile_graph_to_offline_model.md) scenario yet.
- When the raw shape has fewer than four dimensions, converting it to the internal format requires specifying a dimension expansion rule. Different dimension expansion rules result in different data layouts after conversion to the internal format.

## Usage

The following uses the construction of a Graph as an example to describe in detail how to set the internal format of the input RefData node and the output node. The constructed Graph sample is as follows:

![](../figures/specify_internal_format.png)

The sample code is as follows:

```c++
// 1. Construct the RefData node for the entire graph input and set the runtime format.
//  1.1 Construct the tensor desc of RefData.
std::vector<int64_t> weight_shape = {4,50};
TensorDesc weight_desc = TensorDesc(ge::Shape(weight_shape), FORMAT_NHWC, DT_FLOAT);
weight_desc.SetStorageFormat(FORMAT_NC1HWC0); // Set the runtime format of the tensor desc.
weight_desc.SetExpandDimsRule(AscendString("NC")); // Set the dimension expansion rule of the tensor desc.
//  1.2 Construct the RefData node and set its input and output tensor desc.
auto weight = op::RefData("weight").set_attr_index(1);
//  1.3 Set the input and output tensor desc for the RefData node.
weight.update_input_desc_x(refdata_01_desc);
weight.update_output_desc_y(refdata_01_desc);

// 2. Construct other nodes in the graph.
auto fm_data = op::Data("data").set_attr_index(0);

// 3. Construct the Conv2D node for the entire graph output and set the runtime format of the output.
//  3.1 Construct the output tensor desc of Conv2D.
std::vector<int64_t> conv2d_out_shape = {4,50};
TensorDesc conv2d_out_desc = TensorDesc(ge::Shape(conv2d_out_shape), FORMAT_NHWC, DT_FLOAT);
conv2d_out_desc.SetStorageFormat(FORMAT_NC1HWC0); // Set the runtime format of the tensor desc.
conv2d_out_desc.SetExpandDimsRule(AscendString("NC")); // Set the dimension expansion rule of the tensor desc.
//  3.2 Construct the Conv2D node.
auto conv2d = op::Conv2D("conv2d").set_input_x(fm_data).set_input_filter(weight);
//  3.3 Set the output tensor desc for the Conv2D node.
conv2d.update_output_desc_y(conv2d_out_desc);

// 4. Construct the entire graph and set the input and output of the entire network.
ge::Graph graph("demo_graph");
graph.SetInputs({fm_data, weight}).SetOutputs({{conv2d, 0}});
```
