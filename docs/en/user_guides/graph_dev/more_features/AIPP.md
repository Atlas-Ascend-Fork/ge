# AIPP

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-21T09:58:45.003Z pushedAt=2026-10-28T08:24:25.208Z nl=2 -->

This section describes how to enable the AIPP feature during graph construction.

<!-- @ref: ge/res/docs/zh/user_guides/graph_dev/AIPP_res.md#id1 -->

## Feature Overview

AIPP (Artificial Intelligence Pre-Processing) is used to complete image preprocessing on the AI Core, including color space conversion (converting the image format), image normalization (subtracting the mean and multiplying by a coefficient), and cropping (specifying the cropping start point to crop an image of the size required by the neural network). AIPP is classified into static AIPP and dynamic AIPP. Only one of them can be used at a time, and they cannot be supported simultaneously.

- Static AIPP: During model compilation, the AIPP mode is set to static and the AIPP parameters are configured. After the model is generated, the AIPP parameter values are saved in the offline model, and fixed AIPP preprocessing parameters (which cannot be modified) are used in each model inference process.
- Dynamic AIPP: During model compilation, the AIPP mode is set to dynamic. Before each model inference, dynamic AIPP parameter values are set as required before the model is executed, so that different AIPP parameters can be used during model execution. Dynamic AIPP is used in scenarios where preprocessing parameters need to be changed according to service requirements (for example, different cameras use different normalization parameters, and the input image format needs to be compatible with both YUV420 and RGB).

For details about the AIPP function, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md).

## Usage

The following describes how to enable AIPP during model construction:

1. The number of elements in the Channel dimension of the input data of the Data operator must be consistent with the actual image.

    ```c++
    auto shape_data = vector<int64_t>({ 1,10,12,12 });
    // Create a TensorDesc
    TensorDesc desc_data(ge::Shape(shape_data), FORMAT_NCHW, DT_FLOAT16);
    // Create a Data operator (input node)
    auto data = op::Data("data");
    // Assign a value to the input tensor descriptor of the Data operator
    data.update_input_desc_x(desc_data);
    data.update_output_desc_y(desc_data);
    ```

2. Prepare the AIPP configuration file. For configuration details, see [Ascend Tensor Compiler (ATC)](../../atc_tools/README.md). The following provides some configuration examples:

    Example of a static AIPP configuration file:

    ```text
    aipp_op {
    aipp_mode:static          # Indicates static AIPP
    input_format:YUV420SP_U8
    csc_switch:true
    var_reci_chn_0:0.00392157
    var_reci_chn_1:0.00392157
    var_reci_chn_2:0.00392157
    }
    ```

    Example of a dynamic AIPP configuration file (only the following fields need to be configured):

    ```text
    aipp_op {
    aipp_mode:dynamic         # Indicates dynamic AIPP
    related_input_rank: 0     # Optional. Indicates the index of the model input on which AIPP processing is performed, starting from 0. The default value is 0.
    max_src_image_size:752640 # Maximum size of the input image. This parameter is mandatory for dynamic AIPP.
    }
    ```

3. During model compilation, set **INSERT\_OP\_FILE** and **INPUT\_FORMAT** in the options of the aclgrphBuildModel API.

    ```c++
    void PrepareOptions(std::map<AscendString, AscendString>& options) {
        options.insert({
            {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, "1"},
           {ge::ir_option::INSERT_OP_FILE, PATH + "aipp_nv12_img.cfg"},
           {ge::ir_option::INPUT_FORMAT, "NCHW"}
            });
    }
    ```

4. The AIPP operator is automatically inserted into the generated offline model.

    ![](../figures/aipp_op.png)

## Usage Notes

- If the user configures the **static** AIPP feature and also sets the dynamic shape range parameters through INPUT\_SHAPE, the following applies:

    If the model has only one input, this scenario is not supported. If the model has multiple inputs, different input nodes must be configured separately, for example, one input node configured with static AIPP and another node configured with dynamic shape.

- If the user configures the **static AIPP** feature and also sets dynamic resolution through DYNAMIC\_BATCH\_SIZE (the width and height of the input image are uncertain):

    In this scenario, the Crop and Padding functions cannot be enabled in the AIPP configuration file, and the values of src\_image\_size\_w and src\_image\_size\_h in the configuration file must be set to 0.

- If the user configures the **dynamic** AIPP feature and also sets the dynamic shape range parameters through INPUT\_SHAPE, the width and height of the AIPP output must be within the range set by INPUT\_SHAPE.
- If the user configures the **dynamic AIPP** feature and also sets dynamic batch\_size through DYNAMIC\_BATCH\_SIZE:

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP-related parameter values, ensure that batch\_size is set to the maximum batch size.

- If the user configures the **dynamic AIPP** feature and also sets dynamic resolution through DYNAMIC\_IMAGE\_SIZE (the width and height of the input image are uncertain):

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP-related parameter values, the Crop and Padding functions cannot be enabled. In this scenario, also ensure that the width and height set through the aclmdlSetInputAIPP API are equal to the width and height set through the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API, and both must be set to the width and height of the maximum dynamic resolution level.
