# DUMP\_GRAPH\_FORMAT

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-20T12:46:27.919Z pushedAt=2026-10-28T11:28:14.587Z nl=2 -->

## Description

Controls the type of dump files to be generated.

Supported values include:

- onnx: ONNX-based model description structure, which can be opened by visualization software such as Netron. The generated file is named ge\_onnx\*.pbtxt.
- ge\_proto: text file stored in protobuf format. The generated file is named ge\_proto\*.txt.
- readable: highly readable text file similar to the Dynamo fx graph style. The generated file is named ge\_readable\*.txt. For details about the file content, see [readable file parsing](#readable-file-parsing).

Usage:

Configure a string separated by "|", with all types in lowercase. Combined configuration is supported, for example, “ge\_proto|onnx|readable", which means dumping all types of files. Individual configuration is also supported, for example, “ge\_proto”, which means dumping only ge\_proto type files ge\_proto\*.txt.

The DUMP\_GRAPH\_FORMAT environment variable takes effect only when [DUMP\_GE\_GRAPH](DUMP_GE_GRAPH.md) is enabled. The default value is “ge\_proto|onnx".

## Configuration Example

```bash
export DUMP_GRAPH_FORMAT="ge_proto|onnx"
```

## Constraints

- If this environment variable is set to other invalid values, undefined behavior may occur.
- If the operator dump data collection function is enabled, refer to the [ge.exec.enableDump](../../api/graph_engine_api/cpp/ge/options_params/precision_comparison.md) parameter. Even if the value of the DUMP\_GRAPH\_FORMAT environment variable does not include “ge\_proto”, the subgraph ge\_proto\_xxxx\_Build.txt will eventually be dumped.

## readable File Parsing

- The following is a complete ge\_readable\*.txt file:

    ```text
    graph("MakeTransformerSubGraph"):
      %input_0 : [#users=1] = Node[type=Data] (attrs = {index: 0})
      %input_1 : [#users=1] = Node[type=Data] (attrs = {index: 1})
      %input_2 : [#users=1] = Node[type=Data] (attrs = {index: 2})
      %Const_0 : [#users=1] = Node[type=Const] (attrs = {value: [-1 7168]})
      %Reshape_1 : [#users=1] = Node[type=Reshape] (inputs = (x=%input_0, shape=%Const_0), attrs = {axis: 0, num_axes: -1})
      %Cast_2 : [#users=1] = Node[type=Cast] (inputs = (x=%Reshape_1), attrs = {dst_type: 0})
      %Cast_3 : [#users=1] = Node[type=Cast] (inputs = (x=%input_1), attrs = {dst_type: 0})
      %Const_4 : [#users=1] = Node[type=Const] (attrs = {value: [1 0]})
      %Transpose_5 : [#users=1] = Node[type=Transpose] (inputs = (x=%Cast_3, perm=%Const_4))
      %MatMul_6 : [#users=1] = Node[type=MatMul] (inputs = (x1=%Cast_2, x2=%Transpose_5), attrs = {transpose_x1: false, transpose_x2: false})
      %Sigmoid_7 : [#users=1] = Node[type=Sigmoid] (inputs = (x=%MatMul_6))
      %Const_8 : [#users=1] = Node[type=Const] (attrs = {value: [-1 256]})
      %Reshape_9 : [#users=1] = Node[type=Reshape] (inputs = (x=%Sigmoid_7, shape=%Const_8), attrs = {axis: 0, num_axes: -1})
      %Unsqueeze_10 : [#users=1] = Node[type=Unsqueeze] (inputs = (x=%input_2), attrs = {axes: {0}})
      %Cast_11 : [#users=1] = Node[type=Cast] (inputs = (x=%Unsqueeze_10), attrs = {dst_type: 0})
      %Add_12 : [#users=1] = Node[type=Add] (inputs = (x1=%Reshape_9, x2=%Cast_11))
      %Const_13 : [#users=1] = Node[type=Const] (attrs = {value: [2]})
      %TopKV2_14 : [#users=2] = Node[type=TopKV2] (inputs = (x=%Add_12, k=%Const_13), attrs = {sorted: true, dim: -1, largest: true, indices_dtype: 3})
      %ret : [users=1] = get_element[node=%TopKV2_14](0)
      %ret_1 : [users=0] = get_element[node=%TopKV2_14](1)
      %Const_15 : [#users=1] = Node[type=Const] (attrs = {value: [-1]})
      %ReduceSum_16 : [#users=1] = Node[type=ReduceSum] (inputs = (x=%ret, axes=%Const_15), attrs = {keep_dims: false, noop_with_empty_axes: true})
      %Const_17 : [#users=1] = Node[type=Const] (attrs = {value: [4]})
      %TopKV2_18 : [#users=2] = Node[type=TopKV2] (inputs = (x=%ReduceSum_16, k=%Const_17), attrs = {sorted: false, dim: -1, largest: true, indices_dtype: 3})
      %ret_2 : [users=0] = get_element[node=%TopKV2_18](0)
      %ret_3 : [users=1] = get_element[node=%TopKV2_18](1)
      %Cast_19 : [#users=1] = Node[type=Cast] (inputs = (x=%ret_3), attrs = {dst_type: 9})
      %ZerosLike_20 : [#users=1] = Node[type=ZerosLike] (inputs = (x=%ReduceSum_16))
      %Shape_21 : [#users=1] = Node[type=Shape] (inputs = (x=%Cast_19), attrs = {dtype: 3})
      %Const_22 : [#users=1] = Node[type=Const] (attrs = {value: [1.000000]})
      %Cast_23 : [#users=1] = Node[type=Cast] (inputs = (x=%Const_22), attrs = {dst_type: 0})
      %Fill_24 : [#users=1] = Node[type=Fill] (inputs = (dims=%Shape_21, value=%Cast_23))
      %ScatterElements_25 : [#users=1] = Node[type=ScatterElements] (inputs = (data=%ZerosLike_20, indices=%Cast_19, updates=%Fill_24), attrs = {axis: 0, reduction: "none"})
      %Unsqueeze_26 : [#users=1] = Node[type=Unsqueeze] (inputs = (x=%ScatterElements_25), attrs = {axes: {-1}})
      %Const_27 : [#users=1] = Node[type=Const] (attrs = {value: [256 256]})
      %BroadcastTo_28 : [#users=1] = Node[type=BroadcastTo] (inputs = (x=%Unsqueeze_26, shape=%Const_27))
      %Identity_29 : [#users=1] = Node[type=Identity] (inputs = (x=%BroadcastTo_28))
      %Const_30 : [#users=1] = Node[type=Const] (attrs = {value: [256 256]})
      %Reshape_31 : [#users=1] = Node[type=Reshape] (inputs = (x=%Identity_29, shape=%Const_30), attrs = {axis: 0, num_axes: -1})
      %Cast_32 : [#users=1] = Node[type=Cast] (inputs = (x=%Reshape_31), attrs = {dst_type: 12})
      %LogicalNot_33 : [#users=1] = Node[type=LogicalNot] (inputs = (x=%Cast_32))
      %Const_34 : [#users=1] = Node[type=Const] (attrs = {value: [0.000000]})
      %MaskedFill_35 : [#users=1] = Node[type=MaskedFill] (inputs = (x=%Add_12, mask=%LogicalNot_33, value=%Const_34))
      %Const_36 : [#users=1] = Node[type=Const] (attrs = {value: [4]})
      %TopKV2_37 : [#users=2] = Node[type=TopKV2] (inputs = (x=%MaskedFill_35, k=%Const_36), attrs = {sorted: false, dim: -1, largest: true, indices_dtype: 3})
      %ret_4 : [users=0] = get_element[node=%TopKV2_37](0)
      %ret_5 : [users=1] = get_element[node=%TopKV2_37](1)
      %Cast_38 : [#users=1] = Node[type=Cast] (inputs = (x=%ret_5), attrs = {dst_type: 9})
      %GatherElements_39 : [#users=1] = Node[type=GatherElements] (inputs = (x=%Sigmoid_7, index=%Cast_38), attrs = {dim: 1})
      %Const_40 : [#users=1] = Node[type=Const] (attrs = {value: [0.000001]})
      %RealDiv_41 : [#users=1] = Node[type=RealDiv] (inputs = (x1=%GatherElements_39, x2=%Const_40))
      %Const_42 : [#users=1] = Node[type=Const] (attrs = {value: [2.500000]})
      %Mul_43 : [#users=1] = Node[type=Mul] (inputs = (x1=%RealDiv_41, x2=%Const_42))
      %Cast_44 : [#users=1] = Node[type=Cast] (inputs = (x=%Mul_43), attrs = {dst_type: 0})

      return (output_0=%Cast_38, output_1=%Cast_44)
    ```

- The following is a file that contains a subgraph:

    ```text
    graph("TransformerBlockSubgraph"):
      %input_0 : [#users=1] = Node[type=Data] (attrs = {index: 0})
      %pred_1 : [#users=1] = Node[type=Data] (attrs = {index: 1})
      %If_0 : [#users=1] = Node[type=If] (inputs = (cond=%pred_1, input_0=%input_0), attrs = {then_branch: %If_then, else_branch: %If_else})
      %Const_1 : [#users=1] = Node[type=Const] (attrs = {value: [0]})
      %Const_2 : [#users=1] = Node[type=Const] (attrs = {value: [4]})
      %Const_3 : [#users=1] = Node[type=Const] (attrs = {value: [1]})
      %For_6 : [#users=2] = Node[type=For] (inputs = (start=%Const_1, limit=%Const_2, delta=%Const_3, input_1=%If_0), attrs = {body: %For_body})
      %ret : [users=1] = get_element[node=%For_6](0)
      %ret_1 : [users=1] = get_element[node=%For_6](1)

      return (output_0=%ret, output_1=%ret_1)

    graph("If_then"):
      %input_0 : [#users=1] = Node[type=Data] (attrs = {index: 0})
      %Const_0 : [#users=1] = Node[type=Const] (attrs = {value: [0.900000]})
      %Mul_1 : [#users=1] = Node[type=Mul] (inputs = (x1=%input_0, x2=%Const_0))
      return (%Mul_1)

    graph("If_else"):
      %input_0 : [#users=1] = Node[type=Data] (attrs = {index: 0})
      %Identity_0 : [#users=1] = Node[type=Identity] (inputs = (x=%input_0))
      return (%Identity_0)

    graph("For_body"):
      %iter : [#users=1] = Node[type=Data] (attrs = {index: 0})
      %hidden : [#users=1] = Node[type=Data] (attrs = {index: 1})
      %Const_0 : [#users=1] = Node[type=Const] (attrs = {value: [1]})
      %Add_0 : [#users=1] = Node[type=Add] (inputs = (x1=%iter, x2=%Const_0))
      %Const_1 : [#users=1] = Node[type=Const] (attrs = {value: [0.500000]})
      %Mul_1 : [#users=1] = Node[type=Mul] (inputs = (x1=%hidden, x2=%Const_1))
      %Add_2 : [#users=1] = Node[type=Add] (inputs = (x1=%hidden, x2=%Mul_1))

      return (output_0=%Add_0, output_1=%Add_2)
    ```

The fields are described as follows:

- **Graph name**: graph\("< graph name \>"\).
- **Node instance**: %\<node instance name \> : \[\#users=< out-degree \>\] = Node\[type=< node type \>\]\(inputs = \(< input name 1\>=%< input instance 1\>, ...\), attrs = \{< attribute name 1\>: \<attribute value 1\>, ...\}\).
  - \<node instance name \>: node instance name.
  - \#users=< out-degree \>: number of node outputs.
  - Node\[type=< node type \>\]: operator type corresponding to the node. For example, a MatMul node is displayed as Node\[type=MatMul\].
  - inputs = \(< input name 1\>=%< input instance 1\>, ...\): node inputs are displayed in the "parameter name=instance name" form. If parameter name parsing fails, it falls back to the \_input\_N sequence. This field is omitted when the node has no input.

    **Dynamic input**: Dynamic input parameters are numbered in the form of input name \_\#cnt (input name \_0, input name \_1, ...).

  - attrs = \{< attribute name 1\>: \<attribute value 1\>, ...\}: the set of node attributes, including subgraph attribute items and common attribute items. This field is omitted when the attributes are empty.

- **Output reference of a multi-output node**: %ret/ret\_\#cnt : \[\#users=< number of consumers \>\] = get\_element\[node=%< node instance name \>\]\(< output index\>\).
  - %ret/ret\_\#cnt: each output of a multi-output node is uniformly named ret, that is, ret, ret\_1, ret\_2, ...
  - \[\#users=< number of consumers \>\]: the number of consumers of this output.
  - get\_element\[node=%< node instance name \>\]\(< output index\>\): get\_element indicates "extracting the \<output index\>-th output from a multi-output node".

- **Graph output**: return \(< output list \>\)
- Indicates the output of the graph, corresponding to the input of the NetOutput node.
  - Single output: return \(%< output instance \>\).
  - Multiple outputs: return \(output\_0=%< output instance 0\>, output\_1=%< output instance 1\>, ...\). Outputs are numbered in the form of output\_\#cnt (output\_0, output\_1, ...).

- **Subgraph representation**: Nodes that contain subgraphs are represented according to the following rules:
  - **Subgraph declaration**: Declared in the attrs of the parent node: attrs = \{< subgraph attribute name \>: %\<subgraph instance name \>, ... \}; when the subgraph attribute name fails to be parsed, it falls back to the \_graph\_N sequence.
  - **Input correspondence**: The input input\_\#cnt (or args\_\#cnt) of the parent node corresponds to the Data node whose index attribute value is cnt in the subgraph. For example: input\_0 of the parent node → Data\(attrs of the subgraph = \{index: 0\}\).
  - **Output correspondence**:
    - **Single output**: The return value of the subgraph is directly used as the output of the parent node.
    - **Multiple outputs**: The output output\_\#cnt of the subgraph corresponds to the cnt-th output of the parent node, and the parent node extracts the corresponding output through get\_element\[node=%< parent node \>\]\(cnt\).

  - **Subgraph display location**: The subgraph content is displayed separately after the main graph body output ends; each subgraph starts with graph\("< subgraph instance name \>"\):.

## Applicable Products

Full chip support
