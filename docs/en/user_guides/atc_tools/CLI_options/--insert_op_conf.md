# --insert\_op\_conf

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T03:12:35.541Z pushedAt=2026-10-26T02:31:34.818Z -->

## Product Support

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
Supported on all chips.
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--insert_op_conf_res.md#id1 -->

## Feature Description

Path and name of the configuration file for the operator to be inserted, for example, the AIPP preprocessing operator.

If this parameter is used, the input data type is UINT8.

## Related Parameters

- If this parameter is configured, the [--input\_fp16\_nodes](--input_fp16_nodes.md) parameter cannot be used on the same input node at the same time.
- This parameter cannot be used together with [--dynamic\_dims](--dynamic_dims.md).

## Parameter Values

**Parameter value:**
Path and file name of the configuration file for inserting operators.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

**Parameter value constraints:**
The file extension is not limited to the .cfg format, but the content in the configuration file must comply with the prototxt format.

## Recommended Configuration and Benefits

None.

## Example

The following uses inserting an AIPP preprocessing operator as an example. The configuration file content is as follows (the file name is, for example, _insert\_op.cfg_). For details about the AIPP preprocessing configuration file, see [Enabling AIPP](../AIPP/enable_aipp.md).

```textproto
aipp_op {
    aipp_mode:static
    input_format:YUV420SP_U8
    csc_switch:true
    var_reci_chn_0:0.00392157
    var_reci_chn_1:0.00392157
    var_reci_chn_2:0.00392157
}
```

Upload the configured _insert\_op.cfg_ file to any directory on the server where the ATC tool is located, for example, $HOME/module. The usage example is as follows:

```bash
atc --insert_op_conf=$HOME/module/insert_op.cfg ...
```

## Dependencies and Constraints

- If the user configures the **static AIPP** function and also sets the dynamic shape range parameters through [--input\_shape](--input_shape.md), the following applies:

    If the model has only one input, this scenario is not supported. If the model has multiple inputs, different input nodes must be configured, for example, one input node is configured with static AIPP and another node is configured with dynamic shape.

- If the user configures the **static AIPP** function and also sets dynamic resolution (the width and height of the input image are uncertain) through [--dynamic\_image\_size](--dynamic_image_size.md):

    In this scenario, the Crop and Padding functions cannot be enabled in the AIPP configuration file, and the values of src\_image\_size\_w and src\_image\_size\_h in the configuration file must be set to 0.

- If the user configures the **dynamic AIPP** function and also sets the dynamic shape range parameters through [--input\_shape](--input_shape.md), the width and height output by AIPP must be within the range set by [--input\_shape](--input_shape.md).
- If the user configures the **dynamic AIPP** function and also sets dynamic batch\_size through [--dynamic\_batch\_size](--dynamic_batch_size.md):

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP related parameter values, ensure that batch\_size is set to the maximum batch size.

- If the user configures the **dynamic AIPP** function and also sets dynamic resolution (the width and height of the input image are uncertain) through [--dynamic\_image\_size](--dynamic_image_size.md):

    During actual inference, when calling the [aclmdlSetInputAIPP](../../../api/graph_engine_api/c/acl/aclmdlSetInputAIPP.md) API to set the dynamic AIPP related parameter values, the Crop and Padding functions cannot be enabled. In this scenario, ensure that the width and height set through the aclmdlSetInputAIPP API are equal to the width and height set through the [aclmdlSetDynamicHWSize](../../../api/graph_engine_api/c/acl/aclmdlSetDynamicHWSize.md) API, and both must be set to the width and height of the maximum dynamic resolution level.
