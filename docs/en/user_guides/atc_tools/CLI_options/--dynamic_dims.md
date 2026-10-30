# --dynamic\_dims

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:59:23.012Z pushedAt=2026-10-27T06:53:11.605Z -->

## Product Support

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
Supported on all chips
<!-- end id2 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--dynamic_dims_res.md#id1 -->

## Function Description

Sets the gears of dynamic dimensions in ND format. This applies to scenarios where each inference processes dimensions of any size.

To support scenarios where the input format dimensions of networks such as Transformer are uncertain, this parameter is used to set gears for dimensions of any size in ND format. ND indicates that any format is supported.

## Related Parameters

This parameter must be used together with [--input\_shape](--input_shape.md) and [--input\_format](--input_format.md), and cannot be used at the same time as [--dynamic\_batch\_size](--dynamic_batch_size.md), [--dynamic\_image\_size](--dynamic_image_size.md), or [--insert\_op\_conf](--insert_op_conf.md).

## Parameter Value

**Parameter value:**
Set the value in the "dim1,dim2,dim3;dim4,dim5,dim6;dim7,dim8,dim9" format.

**Parameter value format:**
All levels must be enclosed in double quotation marks. Levels are separated by English **semicolons**, and parameters within each level are separated by English **commas**. The dim values in each level correspond to the -1 identified parameters in the [--input\_shape](--input_shape.md) parameter in sequence. Each level must set as many dimensions as the number of -1 in the [--input\_shape](--input_shape.md) parameter.

**Parameter value constraints:**

<!-- npu="A3,910b,910,310p,310b" id3 -->
- For the following products, the number of levels is constrained as follows: the number of levels ranges from \(1,100\], that is, at least 2 levels must be set, and a maximum of 100 levels are supported. It is recommended to configure 3\~4 levels.

    <!-- npu="A3" id5 -->
    Atlas A3 products
    <!-- end id5 -->

    <!-- npu="910b" id6 -->
    Atlas A2 products
    <!-- end id6 -->

    <!-- npu="310b" id7 -->
    Atlas 200I/500 A2 inference product
    <!-- end id7 -->

    <!-- npu="310p" id8 -->
    Atlas inference products
    <!-- end id8 -->

    <!-- npu="910" id9 -->
    Atlas training products
    <!-- end id9 -->
<!-- end id3 -->

<!-- npu="950" id4 -->
- For Ascend 950PR&950DT products, the constraint on the number of levels is as follows: the number of levels ranges from \(1, 256\], that is, at least 2 levels must be set, and a maximum of 256 level configurations are supported. It is recommended to configure 3\~4 levels.
<!-- end id4 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--dynamic_dims_res.md#id2 -->

## Recommended Configuration and Benefits

None.

## Example

- If the network model has only one input:

    The dim values in each level correspond to the parameters identified by -1 in the [--input\_shape](--input_shape.md) parameter in sequence. If there are several -1 in the [--input\_shape](--input_shape.md) parameter, each level must set several dimensions. For example:

    The ATC parameter value is:

    ```bash
    atc --input_shape="data:1,-1"  --dynamic_dims="4;8;16;64" --input_format=ND ...
    ```

    When ATC compiles the model, the supported shapes of the data operator are 1,4; 1,8; 1,16; 1,64.

    The ATC parameter value is:

    ```bash
    atc --input_shape="data:1,-1,-1"  --dynamic_dims="1,2;3,4;5,6;7,8" --input_format=ND
    ```

    When ATC compiles the model, the supported shapes of the data operator are 1,1,2; 1,3,4; 1,5,6; 1,7,8.

- If the network model has multiple inputs:

    The dim values in each level correspond to the parameters identified by -1 in the network model input parameters in sequence. If there are several -1 in the network model input parameters, each level must set several dimensions. For example, the network model has three inputs, namely data\(1,1,40,T\), label\(1,T\), and mask\(T,T\), where T is dynamically variable. The configuration example is:

    ```bash
    atc --input_shape="data:1,1,40,-1;label:1,-1;mask:-1,-1"  --dynamic_dims="20,20,1,1;40,40,2,2;80,60,4,4" --input_format=ND
    ```

    When ATC compiles the model, the supported numbers of input dims combination levels are:

    Level 0: data\(1,1,40,20\)+label\(1,20\)+mask\(1,1\)

    Level 1: data\(1,1,40,40\)+label\(1,40\)+mask\(2,2\)

    Level 2: data\(1,1,40,80\)+label\(1,60\)+mask\(4,4\)

## Dependency Constraints

- **Usage constraints:**

    Networks that contain dynamic shape operators in the process (where the shape of intermediate layers is not fixed) are not supported.

- **API constraints:**

    If dynamic dimensions are set through this parameter during model conversion, before the **model execution** API when performing model inference using the application project:

  - Use the [aclmdlSetInputDynamicDims](../../../api/graph_engine_api/c/acl/aclmdlSetInputDynamicDims.md) API to set the actual dimensions.
  - If the [aclmdlSetInputDynamicDims](../../../api/graph_engine_api/c/acl/aclmdlSetInputDynamicDims.md) API is not used, the model is assigned the maximum value within the dynamic dimension setting range by default during model execution.
