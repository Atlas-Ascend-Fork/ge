# Dynamic Shape

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-16T11:51:46.214Z pushedAt=2026-10-28T07:40:22.213Z nl=2 -->

## INPUT\_SHAPE

Shape information of the model input. This parameter actually corresponds to the `ge.inputShape` option.

**Parameter values:**

- For a model with a static shape, **INPUT\_SHAPE** is an **optional** configuration.
  - If the model has a single input, the shape information is "input\_name:n,c,h,w".
  - If the model has multiple inputs, the shape information is "input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2"; different inputs are separated by **English semicolons**, and input\_name must be the node name in the network model before conversion.

- For a model with a non-static shape, **INPUT\_SHAPE** must be configured.

    If one or more dimension values of the input data in the original model are not fixed, the model can currently be converted by setting shape gears or setting a shape range.

  - Set shape gears (static shape), including setting **batch\_size** gears, setting resolution gears, and setting dynamic dimension gears.

    When setting the INPUT\_SHAPE parameter, set the corresponding dimension value to -1, and use it together with [DYNAMIC\_BATCH\_SIZE](#dynamic_batch_size) (set batch\_size gears) or [DYNAMIC\_IMAGE\_SIZE](#dynamic_image_size) (set resolution gears) or [DYNAMIC\_DIMS](#dynamic_dims) (set dynamic dimension gears).

  - Set the shape range (dynamic shape).

    <!-- npu="310b" id1 -->
    Atlas 200I/500 A2 inference product **does not support setting the shape range**.
    <!-- end id1 -->

    When setting the INPUT\_SHAPE parameter, you can set the value of the corresponding dimension to a range, for example, 1\~10. **The set range value must be valid**.

    - Setting by name is supported: "input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2", for example: "input\_name1:8\~20,3,5,-1;input\_name2:5,3\~9,10,-1". The specified node must be placed in double quotes, and nodes are separated by English semicolons. input\_name must be the node name in the network model before conversion. If you know the name of the data node, setting by name is recommended.
    - Setting by index is supported: "n1,c1,h1,w1;n2,c2,h2,w2", for example: "8\~20,3,5,-1;5,3\~9,10,-1". The node name can be omitted, nodes are arranged in index order, and nodes are separated by English semicolons. When setting the shape range by index, the data node needs to set the index attribute to indicate which input it is, with the index starting from 0.

    If you do not want to specify the range or specific value of a dimension, you can set it to -1, indicating that this dimension can use any value of \>=0. In this scenario, the upper limit of the value is the range of the int64 data type, but it is limited by the physical memory size on the host and device sides. You can accommodate this by increasing the memory.

- The model shape is scalar
  - Non-dynamic gear scenario:

    For an input with a scalar shape, configuration is optional. For example, a model has two inputs, input\_name1 is scalar, that is, the shape is "\[\]", and the input shape of input\_name2 is \[n2,c2,h2,w2\], so the shape information is "**input\_name1:**;input\_name2:n2,c2,h2,w2". Different inputs are separated by **English semicolons**, and input\_name must be the node name in the network model before conversion. If a scalar input is configured, leave it empty.

  - Dynamic gear scenario:

    If the model inputs contain both scalar shapes and shapes that support dynamic gearing, the scalar inputs cannot be ignored and must be configured. For example, if a model has three inputs, namely A:\[-1,c1,h1,w1\], B:\[\], and C:\[n2,c2,h2,w2\], the shape information is "A:-1,c1,h1,w1;**B:**;C:n2,c2,h2,w2", and the scalar input B must be configured.

**Configuration example:**

- Static shape. For example, for the input shape information of a network, input 1: **input\_0\_0 \[16,32,208,208\]**, input 2: **input\_1\_0 \[16,64,208,208\]**, the configuration information of **INPUT\_SHAPE** is:

    ```c++
    {ge::ir_option::INPUT_SHAPE, "input_0_0:16,32,208,208;input_1_0:16,64,208,208"}
    ```

- Non-static shape, static shape:
  - For an example of setting the **batch\_size** gear, see **DYNAMIC\_BATCH\_SIZE**.
  - For an example of setting the resolution gear, see **DYNAMIC\_IMAGE\_SIZE**.
  - For an example of setting the specified dimension gear, see **DYNAMIC\_DIMS**.

- Non-static shape, dynamic shape, example of setting the shape range:

    ```c++
    {ge::ir_option::INPUT_SHAPE, "input_0_0:1~10,32,208,208;input_1_0:16,64,100~208,100~208"}
    ```

    For detailed usage examples and usage notes, see [more features \> dynamic input shape range](../../../../../user_guides/graph_dev/more_features/dynamic_shape.md#dynamic-input-shape-range).

- shape is scalar
  - Non-dynamic gear scenario

    For an input whose shape is scalar, this is optional. For example, a model has two inputs, **input\_name1** is scalar, and the input shape of input\_name2 is \[16,32,208,208\]. The configuration example is:

    ```c++
    {ge::ir_option::INPUT_SHAPE, "input_name1:;input_name2:16,32,208,208"}
    ```

    In the preceding example, **input\_name1** is optional.

  - Dynamic gear scenario

    For an input with a scalar shape, this must be configured. For example, a model has three inputs whose shape information is A:\[-1,32,208,208\], **B:\[\]**, and C:\[16,64,208,208\] respectively. The configuration example is (A is a dynamic gear input; here, the batch_size gear setting is used as an example):

    ```c++
    {ge::ir_option::INPUT_SHAPE, "A:-1,32,208,208;B:;C:16,64,208,208"},
    {ge::ir_option::DYNAMIC_BATCH_SIZE, "1,2,4"}
    ```

> [!NOTE]
>
>If the shape range is set through this parameter during model conversion, when performing model inference using an application project, call the [aclmdlSetDatasetTensorDesc](../../../c/acl/aclmdlSetDatasetTensorDesc.md) API before the **model execution** API to set the actual input Tensor description information (input shape range). After model execution, call the [aclmdlGetDatasetTensorDesc](../../../c/acl/aclmdlGetDatasetTensorDesc.md) API to obtain the Tensor description information of the model's dynamic output. Then call the operation APIs under [aclTensorDesc](../../../c/acl/aclTensorDesc.md) to obtain the memory size occupied by the output Tensor data, the Tensor Format information, and the Tensor dimension information.

**Applicable products:**

Supported on all chips.

## DYNAMIC\_BATCH\_SIZE

Sets the dynamic batch gear parameter, applicable to scenarios where the number of images processed each time is not fixed when performing inference. This parameter actually corresponds to the `ge.dynamicBatchSize` option.

This parameter needs to be used together with INPUT\_SHAPE and cannot be used with DYNAMIC\_IMAGE\_SIZE or DYNAMIC\_DIMS simultaneously. It only supports the scenario where N is in the first position of the shape, that is, the first position of the shape is set to "-1". If N is not in the first position, use the DYNAMIC\_DIMS parameter for configuration.

**Parameter values:** Number of gears, for example, "1,2,4,8".

**Parameter value format**: The specified parameter must be placed in double quotes, and gears are separated by English commas.

<!-- npu="950,A3,910b,910,310p,310b" id48 -->
**Parameter value constraints:**
<!-- end id48 -->

<!-- npu="950" id3 -->
- For Ascend 950PR&950DT series products, the number of gears constraint is: the number of gears value range is \(1, 256\], meaning at least 2 gears must be set, with support for up to 256 gear configurations. The recommended value range for each gear is: \[1\~2048\].
<!-- end id3 -->

<!-- npu="A3,910b,910,310p,310b" id4 -->
- For the following products, the number of gears constraint is: the number of gears value range is \(1,100\], that is, must set at least 2 gears and supports up to 100 gear configurations. The recommended value for each gear is: \[1\~2048\].

    <!-- npu="A3" id5 -->
    Atlas A3 series products
    <!-- end id5 -->

    <!-- npu="310b" id6 -->
    Atlas A2 series products
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
<!-- end id4 -->

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id1 -->

**Configuration example:**

The "-1" in **INPUT\_SHAPE** indicates setting dynamic batch.

```c++
{ge::ir_option::INPUT_FORMAT, "NHWC"},
{ge::ir_option::INPUT_SHAPE, "data:-1,3,416,416"},
{ge::ir_option::DYNAMIC_BATCH_SIZE, "1,2,4,8"}
```

For detailed usage examples and usage notes, see [more features \> dynamic BatchSize](../../../../../user_guides/graph_dev/more_features/dynamic_shape.md#dynamic-batch_size).

**Applicable products:**

<!-- npu="950" id23 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id23 -->
<!-- npu="A3" id2 -->
- Atlas A3 series products: Supported
<!-- end id2 -->
<!-- npu="910b" id37 -->
- Atlas A2 series products: Supported
<!-- end id37 -->
<!-- npu="310b" id45 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id45 -->
<!-- npu="310p" id49 -->
- Atlas inference products: Supported
<!-- end id49 -->
<!-- npu="910" id50 -->
- Atlas training products: Supported
<!-- end id50 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id2 -->

## DYNAMIC\_IMAGE\_SIZE

Sets the dynamic resolution parameter of the input image. It is applicable to scenarios where the width and height of each processed image are not fixed when performing inference. The **options** parameter that this parameter actually corresponds to is `ge.dynamicImageSize`.

This parameter needs to be used together with **INPUT\_SHAPE** and cannot be used with **DYNAMIC\_BATCH\_SIZE** or **DYNAMIC\_DIMS** simultaneously.

**Parameter values:**  "imagesize1\_height,imagesize1\_width;imagesize2\_height,imagesize2\_width".

**Parameter value format:** The specified parameter must be placed in double quotes. Gears are separated by English **semicolons**, and parameters within each gear are separated by English **commas**.

<!-- npu="950,A3,910b,910,310p,310b" id46 -->
**Parameter value constraints:**
<!-- end id46 -->

<!-- npu="950" id10 -->
- For Ascend 950PR&950DT series products, the number of gears is constrained as follows: the value range of the number of gears is \(1, 256\], that is, you must configure at least 2 gears and can configure up to 256 gears.
<!-- end id10 -->

<!-- npu="A3,910b,910,310p,310b" id11 -->
- For the following products, the number of gears is constrained as follows: the value range of the number of gears is \(1,100\], that is, you must configure at least 2 gears and at most 100 gears

    <!-- npu="A3" id12 -->
    Atlas A3 series products
    <!-- end id12 -->

    <!-- npu="910b" id13 -->
    Atlas A2 series products
    <!-- end id13 -->

    <!-- npu="310b" id14 -->
    Atlas 200I/500 A2 inference product
    <!-- end id14 -->

    <!-- npu="310p" id15 -->
    Atlas inference products
    <!-- end id15 -->

    <!-- npu="910" id16 -->
    Atlas training products
    <!-- end id16 -->
<!-- end id11 -->

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id3 -->

**Configuration example:**

The "-1" in **INPUT\_SHAPE** indicates setting dynamic resolution.

```c++
{ge::ir_option::INPUT_FORMAT, "NCHW"},
{ge::ir_option::INPUT_SHAPE, "data:8,3,-1,-1"},
{ge::ir_option::DYNAMIC_IMAGE_SIZE, "416,416;832,832"}
```

For detailed usage examples and usage notes, see [more features \> dynamic resolution](../../../../../user_guides/graph_dev/more_features/dynamic_shape.md#dynamic-resolution).

**Applicable products:**

<!-- npu="950" id17 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id17 -->
<!-- npu="A3" id18 -->
- Atlas A3 series products: Supported
<!-- end id18 -->
<!-- npu="910b" id19 -->
- Atlas A2 series products: Supported
<!-- end id19 -->
<!-- npu="310b" id20 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id20 -->
<!-- npu="310p" id21 -->
- Atlas inference products: Supported
<!-- end id21 -->
<!-- npu="910" id22 -->
- Atlas training products: Supported
<!-- end id22 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id4 -->

## DYNAMIC\_DIMS

Sets the gear of dynamic dimensions in ND format. Applicable to scenarios where each processing handles arbitrary dimensions when performing inference. This parameter actually corresponds to the `ge.dynamicDims` option.

This parameter needs to be used together with **INPUT\_SHAPE** and cannot be used with **DYNAMIC\_BATCH\_SIZE** or **DYNAMIC\_IMAGE\_SIZE** simultaneously.

**Parameter values:** Set in the form of "dim1,dim2,dim3;dim4,dim5,dim6;dim7,dim8,dim9".

**Parameter value format**: All gears must be placed in double quotes, gears are separated by English **semicolons**, and parameters within each gear are separated by English **commas**; the dim values in each gear correspond in sequence to the parameters identified by -1 in the **INPUT\_SHAPE** parameter. The number of dimensions set in each gear must equal the number of -1 in the **INPUT\_SHAPE** parameter.

<!-- npu="950,A3,910b,910,310p,310b" id47 -->
**Parameter value constraints:**
<!-- end id47 -->

<!-- npu="950" id24 -->
- For Ascend 950PR&950DT series products, the number of gears constraint is: the number of gears value range is \(1, 256\], that is, must set at least 2 gears, supports up to 256 gear configurations, and it is recommended to configure 3\~4 gears.
<!-- end id24 -->

<!-- npu="A3,910b,910,310p,310b" id25 -->
- For the following products, the number of gears constraint is: the number of gears value range is \(1,100\], that is, must set at least 2 gears, supports up to 100 gear configurations, and it is recommended to configure 3\~4 gears.

    <!-- npu="A3" id26 -->
    Atlas A3 series products
    <!-- end id26 -->

    <!-- npu="910b" id27 -->
    Atlas A2 series products
    <!-- end id27 -->

    <!-- npu="310b" id28 -->
    Atlas 200I/500 A2 inference product
    <!-- end id28 -->

    <!-- npu="310p" id29 -->
    Atlas inference products
    <!-- end id29 -->

    <!-- npu="910" id30 -->
    Atlas training products
    <!-- end id30 -->

<!-- end id25 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id5 -->

**Configuration example:**

```c++
{ge::ir_option::INPUT_FORMAT, "ND"},
{ge::ir_option::INPUT_SHAPE, "data:1,-1"},
{ge::ir_option::DYNAMIC_DIMS, "4;8;16;64"}
// During model compilation, the supported shapes of the data operator are 1,4; 1,8; 1,16;1,64.
{ge::ir_option::INPUT_FORMAT, "ND"},
{ge::ir_option::INPUT_SHAPE, "data:1,-1,-1"},
{ge::ir_option::DYNAMIC_DIMS, "1,2;3,4;5,6;7,8"}
// During model compilation, the supported shapes of the data operator are 1,1,2; 1,3,4; 1,5,6; 1,7,8.
```

For detailed usage examples and usage notes, see [more features \> dynamic dimension](../../../../../user_guides/graph_dev/more_features/dynamic_shape.md#dynamic-dimension).

**Applicable products:**

<!-- npu="950" id31 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id31 -->
<!-- npu="A3" id32 -->
- Atlas A3 series products: Supported
<!-- end id32 -->
<!-- npu="910b" id33 -->
- Atlas A2 series products: Supported
<!-- end id33 -->
<!-- npu="310b" id34 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id34 -->
<!-- npu="310p" id35 -->
- Atlas inference products: Supported
<!-- end id35 -->
<!-- npu="910" id36 -->
- Atlas training products: Supported
<!-- end id36 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id6 -->

## AC\_PARALLEL\_ENABLE

<!-- npu="950,A3,910b,910,310p" id38 -->
In a dynamic shape graph, whether AI CPU operators and AI Core operators are allowed to run in parallel. This parameter actually corresponds to the `ac_parallel_enable` option.

In a dynamic shape graph, when the switch is enabled, the system automatically identifies AI CPU operators that can run concurrently with the AI Core, and dispatches operators of different engines to different streams to achieve parallelism among multiple engines, thereby improving resource utilization efficiency and dynamic shape execution performance.

**Parameter values:**

- **1**: Allows parallel execution between AI CPU and AI Core operators.
- **0** (default): AI CPU operators are not dispatched to a separate stream.

**Configuration example:**

```c++
{ge::ir_option::AC_PARALLEL_ENABLE, "1"}
```
<!-- end id38 -->

<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id7 -->

**Applicable products:**

<!-- npu="950" id39 -->
- Ascend 950PR&950DT series products: Supported
<!-- end id39 -->
<!-- npu="A3" id40 -->
- Atlas A3 series products: Supported
<!-- end id40 -->
<!-- npu="910b" id41 -->
- Atlas A2 series products: Supported
<!-- end id41 -->
<!-- npu="310b" id42 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id42 -->
<!-- npu="310p" id43 -->
- Atlas inference products: Supported
<!-- end id43 -->
<!-- npu="910" id44 -->
- Atlas training products: Supported
<!-- end id44 -->
<!-- @ref: ge/res/docs/en/api/graph_engine_api/cpp/ge/aclgrphBuildModel_config_params/dynamic_shape_res.md#id8 -->
