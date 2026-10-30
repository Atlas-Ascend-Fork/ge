# --input\_shape

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:11:58.835Z pushedAt=2026-10-26T02:24:45.251Z -->

## Product Support

Supported on all chips

## Function Description

Specifies the shape of the model input data.

## Related Parameters

In the dynamic grading scenario, use the [--dynamic\_batch\_size](--dynamic_batch_size.md) (set batch\_size level), [--dynamic\_image\_size](--dynamic_image_size.md) (set resolution level), or [--dynamic\_dims](--dynamic_dims.md) (set specified dimension level) parameter.

## Parameter Values

**Parameter value:**

- If the original model has a fixed shape, the --input\_shape parameter is an **optional** configuration.
  - If the model has a single input, the shape example is `input_name:n,c,h,w`.
  - If the model has multiple inputs, separate the shapes with English semicolons. The example is `input_name1:n1,c1,h1,w1;input_name2:n2,c2,h2,w2`.

- If the original model has a non-fixed shape, the --input\_shape parameter is a **mandatory** configuration.

    If one or more dimension values of the input data in the original model are not fixed, the model can currently be converted in either of the following two ways: setting dynamic grading or setting a shape range.

  - Set dynamic grading (static shape), including setting the batch\_size level, setting the resolution level, and setting the specified dimension level.

    When setting the --input\_shape parameter, set the corresponding dimension value to -1, and use it together with the [--dynamic\_batch\_size](--dynamic_batch_size.md) (set the batch\_size level), [--dynamic\_image\_size](--dynamic_image_size.md) (set the resolution level), or [--dynamic\_dims](--dynamic_dims.md) (set the specified dimension level) parameter.

  - Set a shape range (dynamic shape).

    <!-- npu="310b" id1 -->
    **The Atlas 200I/500 A2 inference product does not support setting the shape range.**
    <!-- end id1 -->

    When setting the --input\_shape parameter, you can set the value of the corresponding dimension to a range, for example, 1\~10. **The set range value must be valid.**

    If you do not want to specify the range or a specific value of a dimension, you can set it to -1. During model execution, the dimension is resolved to any value of \>=0.

- The shape of the original model is scalar
  - Non-dynamic grading scenario:

    For an input whose shape is scalar, this is an optional configuration. For example, if a model has two inputs, input\_name1 is scalar, that is, the shape is in the "\[\]" form, and the shape example of the input\_name2 input is \[n2,c2,h2,w2\], the shape information is `input_name1:;input_name2:n2,c2,h2,w2`. If a scalar input is configured, it is configured as empty.

  - Dynamic grading scenario:

    If the model inputs include both a scalar shape and a shape that supports dynamic grading, the scalar input cannot be ignored and is a mandatory configuration. For example, if a model has three inputs, namely `A:[-1,c1,h1,w1]`, `B:[]`, and `C:[n2,c2,h2,w2]`, the shape information is `A:-1,c1,h1,w1;B:;C:n2,c2,h2,w2`, and the scalar input B is a mandatory configuration.

**Parameter value constraints:**

- If a model has multiple inputs, the specified nodes must be enclosed in double quotation marks, and different inputs must be separated by **English semicolons**. input\_name must be the node name in the network model before conversion.
- If a dimension value of the input data in the original model is not fixed (for example, input\_name1:?, h, w, c), after opening the model with visualization software such as Netron, the input information example is as follows:

    ![](../figures/input_info_example.png)

    In this scenario, the --input\_shape parameter is mandatory, and you can perform the following operations:

  - Fix the shape by setting the dimension value to a fixed value, for example, input\_name1:**1**,h,w,c, to convert the original model whose input data has an unfixed dimension into an offline model with fixed dimensions.
  - Set shape grading, for example, set it to "-1", and use it together with the [--dynamic\_batch\_size](--dynamic_batch_size.md) parameter.

- When setting the shape range, if it is set to -1, it indicates that this dimension can use any value of \>=0. In this scenario, the upper limit of the value is the range represented by the int64 data type, but it is limited by the physical memory size on the host and device sides. Users can support it by increasing the memory.
- If the AIPP function is also set through [--insert\_op\_conf](--insert_op_conf.md) when using this parameter, the width and height of the AIPP output image must be within the range set by this parameter.

## Recommended Configuration and Benefits

None.

## Example

- Fixed shape, --input\_shape optional configuration

    For example, the input shape information of a network is as follows: input 1: `input_0_0 [16,32,208,208]`, input 2: `input_1_0 [16,64,208,208]`. The --input\_shape configuration information is:

    ```bash
    atc --input_shape="input_0_0:16,32,208,208;input_1_0:16,64,208,208" ...
    ```

- Non-fixed shape, --input\_shape mandatory configuration
  - Static shape
    - For an example of setting batch\_size levels, see also [--dynamic\_batch\_size](--dynamic_batch_size.md).
    - For an example of setting resolution levels, see also [--dynamic\_image\_size](--dynamic_image_size.md).
    - For an example of setting specified dimension levels, see also [--dynamic\_dims](--dynamic_dims.md).

  - Dynamic shape

    Example of setting the shape range:

    ```bash
    atc --input_shape="input_0_0:1~10,32,208,208;input_1_0:16,64,100~208,100~208"  ...
    ```

- shape is scalar
  - Non-dynamic grading scenario

    For an input whose shape is scalar, this is an optional configuration. For example, a model has two inputs, **input\_name1** is scalar, and the input shape of input\_name2 is \[16,32,208,208\]. The configuration example is:

    ```bash
    atc --input_shape="input_name1:;input_name2:16,32,208,208" ...
    ```

    In the preceding example, **input\_name1** is an optional configuration**.**

  - Dynamic grading scenario

    For an input whose shape is scalar, this is a mandatory configuration. For example, a model has three inputs whose shape information is A:\[-1,32,208,208\], **B:\[\]**, and C:\[16,64,208,208\] respectively. The configuration example is (A is a dynamic grading input, and here setting batch\_size levels is used as an example):

    ```bash
    atc --input_shape="A:-1,32,208,208;B:;C:16,64,208,208"  --dynamic_batch_size="1,2,4" ...
    ```

## Dependencies and Constraints

- **Usage constraints:**
  - If the user sets the dynamic shape range parameter through [--input\_shape](--input_shape.md) and also configures the dynamic AIPP function through the [--insert\_op\_conf](--insert_op_conf.md) parameter, the width and height of the AIPP output must be within the range set by [--input\_shape](--input_shape.md).
  - If the user sets the dynamic shape range parameter through [--input\_shape](--input_shape.md) and also configures the static AIPP function through the [--insert\_op\_conf](--insert_op_conf.md) parameter, the following applies:

    If the model has only one input, this scenario is not supported. If the model has multiple inputs, different input nodes must be configured separately. For example, one input node is configured with static AIPP, and another node is configured with dynamic shape.

- **API constraints:**

    If the shape range is set through this parameter during model conversion, when performing model inference using an application project, call the [aclmdlSetDatasetTensorDesc](../../../api/graph_engine_api/c/acl/aclmdlSetDatasetTensorDesc.md) API before the [aclmdlExecute](../../../api/graph_engine_api/c/acl/aclmdlExecute.md) API to set the actual input tensor description (input shape range). After model execution, call the [aclmdlGetDatasetTensorDesc](../../../api/graph_engine_api/c/acl/aclmdlGetDatasetTensorDesc.md) API to obtain the tensor description of the model's dynamic output. Then call the operation APIs under [aclTensorDesc](../../../api/graph_engine_api/c/acl/aclTensorDesc.md) to obtain the memory size occupied by the output tensor data, the tensor format information, and the tensor dimension information.
