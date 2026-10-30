# --out\_nodes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:34:06.686Z pushedAt=2026-10-27T01:21:23.084Z -->

## Product Support

All chips supported

## Function Description

Specifies the output node (operator) of a layer as the output of the network model, or specifies the name of the network model output.

If no output node is specified, the model output defaults to the operator information of the last layer. If an output node is specified, the specified one takes effect. In some cases, users may want to view the output of an intermediate-layer operator. In this case, the parameter can be used to specify the output of a certain layer operator during model conversion. After model conversion, the parameter information of the specified output operator can be viewed at the end of the corresponding OM offline model file. If the information cannot be viewed through the OM offline model file, it can be converted to JSON format for viewing.

## Related Parameters

When [--framework](--framework.md) is set to 1 and the network model is a MindSpore framework model, setting this parameter is invalid, but the model conversion still succeeds.

## Parameter Value

**Parameter value:**

- Specify the name of a node (node\_name) in the network model.

    If multiple output nodes are specified, their names must be enclosed in double quotation marks and separated by semicolons. node\_name must be the node name in the network model before model conversion. The number after the colon indicates the ordinal number of the output. For example, node\_name1:0 indicates the first output of the node named node\_name1.

- Specify the topname of the output node (this scenario applies only to Caffe network models).

    If multiple output nodes are specified, their topnames must be enclosed in double quotation marks and separated by semicolons. The topname must be the top name of a layer in the Caffe network model before compilation. If several layers have the same topname, the topname of the last layer is used as the final effective value.

- Specify the name of the network model output (the name of output) (this scenario applies only to ONNX network models).

    If multiple outputs are specified, their names must be enclosed in double quotation marks and separated by semicolons. The output must be an output of the network model.

**Parameter value constraints:**

1. Of the three methods for specifying the parameter value, only one can be used at a time. They cannot be used simultaneously.
2. If the parameter value is the topname of an output node of a layer, this method is limited to Caffe network models.
3. If the parameter value is the name of the network model output (the name of output), this method is limited to ONNX network models.

## Recommended Configuration and Benefits

None.

## Example

- The parameter value is the name of a node (node\_name) in the network model.

    ```bash
    atc --out_nodes="node_name1:0;node_name1:1;node_name2:0" ...
    ```

- The parameter value is the topname of the output node of a layer.

    ```bash
    atc --out_nodes="topname0;topname1;topname2" ...
    ```

- The parameter value is the name of the network model output (the name of output).

    ```bash
    atc --out_nodes="output1;output2;output3" ...
    ```

## Dependencies and Constraints

None.
