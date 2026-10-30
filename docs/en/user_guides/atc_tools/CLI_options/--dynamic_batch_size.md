# --dynamic\_batch\_size

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T02:58:33.228Z pushedAt=2026-10-23T13:52:53.318Z -->

## Product Support

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
All chips are supported
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--dynamic_batch_size_res.md#id1 -->

## Function Description

Sets the dynamic batch\_size parameter, which applies to inference scenarios where the number of images or sentences processed each time is not fixed.

In certain inference scenarios, such as running a target recognition network after targets are detected, the input batch\_size of the target recognition network is not fixed because the number of targets varies. If each inference is computed based on the maximum batch\_size or maximum resolution, computing resources are wasted. Therefore, inference needs to support dynamic batch\_size and dynamic resolution scenarios. When using the ATC tool, set the supported batch\_size through this parameter, and set the supported resolution levels through the [--dynamic\_image\_size](--dynamic_image_size.md) parameter.

After model conversion is complete, an additional input is added to the generated offline om model. During model inference, the specific batch\_size value is provided through this additional input. For example, if the batch\_size of input a is dynamic, the offline om model contains an input b corresponding to a to describe the specific batch\_size of a.

## Related Parameters

- This parameter must be used together with [--input\_shape](--input_shape.md) and cannot be used together with [--dynamic\_image\_size](--dynamic_image_size.md), [--dynamic\_dims](--dynamic_dims.md), or [--input\_shape\_range](--input_shape_range.md). It supports only the scenario where N is in the first position of the shape, that is, the first position of the shape is set to "-1". If N is not in the first position, use the [--dynamic\_dims](--dynamic_dims.md) parameter for configuration.

## Parameter Values

**Parameter value:**
Number of gears, for example, "1,2,4,8".

**Parameter value format:**
The specified parameter must be enclosed in double quotation marks, and gears must be separated by commas.

**Parameter value constraints:**

<!-- npu="A3,910b,910,310p,310b" id3 -->
- For the following products, the gear number constraints are as follows: the gear number ranges from \(1,100\], that is, at least 2 gears must be set and a maximum of 100 gears are supported; the recommended value of each gear is limited to: \[1\~2048\].

    <!-- npu="A3" id5 -->
    Atlas A3 products
    <!-- end id5 -->

    <!-- npu="910b" id6 -->
    Atlas A2 products
    <!-- end id6 -->

    <!-- npu="310b" id7 -->
    Atlas 200I/500 A2 inference products
    <!-- end id7 -->

    <!-- npu="310p" id8 -->
    Atlas inference products
    <!-- end id8 -->

    <!-- npu="910" id9 -->
    Atlas training products
    <!-- end id9 -->
<!-- end id3 -->
<!-- npu="950" id4 -->
- For the Ascend 950PR&950DT products, the constraint on the number of gears is as follows: the value range of the number of gears is \(1, 256\], that is, at least 2 gears must be set, and a maximum of 256 gears are supported; the recommended value of each gear is limited to: \[1\~2048\].
<!-- end id4 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--dynamic_batch_size_res.md#id2 -->

- If the gear values set by the user are too large or there are too many gears, when performing inference in the runtime environment, it is recommended to run the **swapoff -a** command to disable the use of the swap partition as memory, so as to prevent the runtime environment from becoming extremely slow because the swap space is used as memory when the memory is insufficient.

## Recommended Configuration and Benefits

- If the gear values set by the user are too large or there are too many gears, model conversion may fail. In this case, the user is advised to reduce the number of gears or lower the gear values.
- For CV (computer vision) networks, [--dynamic\_batch\_size](--dynamic_batch_size.md) the recommended values are 8 and 16 gears. In this scenario, the network performance is better than that of a single batch\_size (8 and 16 gears are only recommended values; in actual use, refer to the actual test results).
- For OCR/NLP (optical character recognition/natural language processing) networks, the recommended gear values for [--dynamic\_batch\_size](--dynamic_batch_size.md) are integer multiples of 16 (these values are only recommended values; in actual use, refer to the actual test results).

## Example

```bash
atc --input_shape="data:-1,3,416,416;img_info:-1,4"  --dynamic_batch_size="1,2,4,8" ...
```

Here, "-1" in “--input\_shape” indicates setting the dynamic batch\_size. Therefore, during model compilation, ATC supports the following input combination gears:

Gear 0: data\(1,3,416,416\)+img\_info\(1,4\)

Gear 1: data\(2,3,416,416\)+img\_info\(2,4\)

Gear 2: data\(4,3,416,416\)+img\_info\(4,4\)

Gear 3: data\(8,3,416,416\)+img\_info\(8,4\)

## Dependencies and Constraints

- **Usage constraints:**
  - Networks that contain operators with dynamic shapes in the process (the shape of the intermediate network layer is not fixed) are not supported.
  - If the user sets dynamic batch\_size and also enables the dynamic AIPP feature through the [--insert\_op\_conf](--insert_op_conf.md) parameter:

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP-related parameter values, ensure that batch\_size is set to the maximum batch number.

  - After the dynamic batch\_size feature is set through this parameter, the network structure of the generated offline model differs from that in the fixed batch\_size scenario, and inference performance may vary.

- **API constraints:**

    If the dynamic batch\_size is set through this parameter during model conversion, before the **model execution** API when performing inference using the application project:

  - Use the [aclmdlSetDynamicBatchSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicBatchSize.md) API to set the actual batch\_size level.
  - If the [aclmdlSetDynamicBatchSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicBatchSize.md) API is not used, the model is assigned the maximum value in the batch\_size setting range by default during model execution.
