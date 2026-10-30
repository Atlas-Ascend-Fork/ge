# --input\_shape\_range

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:11:23.423Z pushedAt=2026-10-26T01:54:54.563Z -->

## Function Description

**This parameter is deprecated. Do not use it. To specify the shape range of model input data, use [--input\_shape](--input_shape.md).**

Specifies the shape range of model input data.

## Related Parameters

This parameter cannot be used together with [--dynamic\_batch\_size](--dynamic_batch_size.md), [--dynamic\_image\_size](--dynamic_image_size.md), or [--dynamic\_dims](--dynamic_dims.md).

## Parameter Value Range

**Parameter value:**
Shape range information of the model input data, for example: "input\_name1:\[n1\~n2,c1,h1,w1\];input\_name2:\[n2,c2,h2,w2\]". The specified node must be enclosed in double quotation marks, and multiple nodes must be separated by semicolons. input\_name must be the node name in the network model before conversion.

**Parameter value constraints:**

- The shape range information must be enclosed in English \[\].
- This parameter does not limit the number of dimensions. Any value in a dimension can be specified by the user as a value range.
- If the user does not want to specify the value of a dimension, set it to -1, indicating that this dimension can use any value of \>=0.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --input_shape_range="input1:[8~20,3,5,-1];input2:[5,3~9,10,-1]" ...
```

## Dependency Constraints

- **Usage Constraints:**
  - This parameter applies only to TensorFlow and ONNX network models.
  - If AIPP is configured through [--insert\_op\_conf](--insert_op_conf.md) while this parameter is used, the width and height of the AIPP output image must fall within the range set by [--input\_shape\_range](--input_shape_range.md).

- **API Constraints:**

    If the shape range is set through this parameter during model conversion, when performing model inference using the application project, you need to call [aclmdlSetDatasetTensorDesc](../../../api/graph_engine_api/c/acl/aclmdlSetDatasetTensorDesc.md) before [aclmdlExecute](../../../api/graph_engine_api/c/acl/aclmdlExecute.md) to set the actual input tensor description (input shape range). After model execution, call [aclmdlGetDatasetTensorDesc](../../../api/graph_engine_api/c/acl/aclmdlGetDatasetTensorDesc.md) to obtain the tensor description of the model's dynamic output, and then call the operation APIs under aclTensorDesc to obtain the memory size occupied by the output tensor data, the tensor Format information, the tensor dimension information, and so on.
