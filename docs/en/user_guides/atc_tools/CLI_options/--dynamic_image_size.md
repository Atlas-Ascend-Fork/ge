# --dynamic\_image\_size

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:59:30.521Z pushedAt=2026-10-27T07:05:21.779Z -->

## Product Support Status

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
All chips are supported.
<!-- end id2 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--dynamic_image_size_res.md#id1 -->

## Function Description

Sets the dynamic resolution parameter of the input image. This applies to scenarios where the width and height of each image are not fixed during inference.

## Related Parameters

- This parameter must be used together with [--input\_shape](--input_shape.md), and cannot be used simultaneously with [--dynamic\_batch\_size](--dynamic_batch_size.md) or [--dynamic\_dims](--dynamic_dims.md).
- When this parameter is used to set dynamic resolution, the [--input\_format](--input_format.md) parameter supports only NCHW and NHWC. For other format scenarios, use the [--dynamic\_dims](--dynamic_dims.md) parameter to set the resolution.

## Parameter Values

**Parameter value:**
Dynamic resolution parameter, for example, "imagesize1\_height,imagesize1\_width;imagesize2\_height,imagesize2\_width".

**Parameter value format:**
The specified parameter must be enclosed in double quotation marks. Levels are separated by English **semicolons**, and parameters within each level are separated by English **commas**.

**Parameter value constraints:**

<!-- npu="A3,910b,910,310p,310b" id3 -->
- For the following products, the number of levels is constrained as follows: the number of levels ranges from \(1,100\], that is, at least 2 levels must be set, and a maximum of 100 levels are supported.

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
- For the Ascend 950PR&950DT products, the number of gears is constrained as follows: the value range of the number of gears is \(1, 256\], that is, at least 2 gears must be set, and a maximum of 256 gears are supported.
<!-- end id4 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--dynamic_image_size_res.md#id2 -->

- If the user sets an excessively large resolution value or too many gears, when performing inference in the runtime environment, it is recommended to run the **swapoff -a** command to disable the use of the swap space as memory, preventing the situation where the swap space is used as memory due to insufficient memory, which causes the runtime environment to become abnormally slow.

## Recommended Configuration and Benefits

If the user sets an excessively large resolution value or too many levels, model conversion may fail. In this case, it is recommended that the user reduce the number of levels or lower the level values.

## Example

```bash
atc --input_shape="data:8,3,-1,-1;img_info:8,4,-1,-1"  --dynamic_image_size="416,416;832,832" ...
```

Here, "-1" in “--input\_shape” indicates setting dynamic resolution. When ATC compiles the model, the supported input combination levels are as follows:

Level 0: data\(8,3,416,416\)+img\_info\(8,4,416,416\)

Level 1: data\(8,3,832,832\)+img\_info\(8,4,832,832\)

## Dependency Constraints

- **Usage constraints:**
  - Networks that contain dynamic shape operators in the process (the shape of the network intermediate layers is not fixed) are not supported.
  - If the user sets dynamic resolution, ensure that the resolutions of different levels can be inferred normally in the native framework.
  - If the user set dynamic resolution, during actual inference, the size of the dataset images used must match the specific resolution in use.
  - If the user sets dynamic resolution, that is, the width and height of the input image are not fixed, and the static AIPP function is also set through the [--insert\_op\_conf](--insert_op_conf.md) parameter: in this scenario, the Crop and Padding functions cannot be enabled in the AIPP configuration file, and the values of src\_image\_size\_w and src\_image\_size\_h in the configuration file must be set to 0.
  - If the user set dynamic resolution, and the dynamic AIPP function is also set through the [--insert\_op\_conf](--insert_op_conf.md) parameter:

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP-related parameter values, the Crop and Padding functions cannot be enabled. In this scenario, you also need to ensure that the width and height set through the aclmdlSetInputAIPP API are equal to the width and height set through the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API, and both must be set to the width and height of the maximum dynamic resolution gear.

  - After the dynamic resolution feature is set through this parameter, the network structure of the generated offline model differs from that in the fixed resolution scenario, and the inference performance may differ.

- **API constraints:**

    If dynamic resolution is set through this parameter during model conversion, before the **model execution** API when using the application project for model inference:

  - Use the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API to set the actual resolution. During actual inference, the size of the dataset images used must match the specific resolution.
  - If the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API is not used, the model execution assigns values based on the width and height of the maximum level in the dynamic resolution setting range by default.
