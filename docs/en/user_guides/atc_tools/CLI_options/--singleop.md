# --singleop

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-22T03:41:53.535Z pushedAt=2026-10-27T12:28:28.646Z -->

## Product Support

<!-- npu="950,A3,910b,910,310p,310b" id15 -->
Supported on all chips
<!-- end id15 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--singleop_res.md#id1 -->

## Feature Description

The single operator description file converts a single operator description file (in JSON format) into an offline model adapted to the AI processor, so that subsequent single operator function verification can be performed.

Compatibility notes:

For dynamic shape operator scenarios and Ascend virtualization instance scenarios, the CANN software package version of the om offline model conversion environment must be the same as that of the runtime environment.

(An Ascend virtualization instance mainly refers to the scenario of NPU resource virtualization. Its main purpose is to improve the utilization of physical NPU resources through mutually isolated vNPU virtual instances.)

The support for Ascend virtualization instances is as follows:

<!-- npu="950" id5 -->
- Ascend 950PR&950DT products: Not Supported
<!-- end id5 -->

<!-- npu="A3" id6 -->
- Atlas A3 products: Not Supported
<!-- end id6 -->

<!-- npu="910b" id4 -->
- Atlas A2 products: Supported
<!-- end id4 -->

<!-- npu="310b" id7 -->
- Atlas 200I/500 A2 inference product: Not Supported
<!-- end id7 -->

<!-- npu="310p" id2 -->
- Atlas inference products: Supported
<!-- end id2 -->

<!-- npu="910" id3 -->
- Atlas training products: Supported
<!-- end id3 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--singleop_res.md#id2 -->

## Related Parameters

When this parameter is used, only the following parameters can be used with it, among which [--output](--output.md) and [--soc\_version](--soc_version.md) are required.

<!-- npu="950" id14 -->
- Parameters that can be used with the Ascend 950PR&950DT products:
  - [--output](--output.md)
  - [--soc\_version](--soc_version.md)
  - [--precision\_mode](--precision_mode.md)
  - [--precision\_mode\_v2](--precision_mode_v2.md)
  - [--op\_select\_implmode](--op_select_implmode.md)
  - [--optypelist\_for\_implmode](--optypelist_for_implmode.md)
  - [--enable\_small\_channel](--enable_small_channel.md)
  - [--sparsity](--sparsity.md)
  - [--allow\_hf32](--allow_hf32.md)
  - [--log](--log.md)
  - [--debug\_dir](--debug_dir.md)
  - [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md)
  - [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md)
  - [--atomic\_clean\_policy](--atomic_clean_policy.md)
  - [--customize\_dtypes](--customize_dtypes.md)
  - [--is\_weight\_clip](--is_weight_clip.md)
<!-- end id14 -->

<!-- npu="A3" id13 -->
- Parameters that can be used with Atlas A3 products:
  - [--output](--output.md)
  - [--soc\_version](--soc_version.md)
  - [--precision\_mode](--precision_mode.md)
  - [--precision\_mode\_v2](--precision_mode_v2.md)
  - [--op\_select\_implmode](--op_select_implmode.md)
  - [--optypelist\_for\_implmode](--optypelist_for_implmode.md)
  - [--enable\_small\_channel](--enable_small_channel.md)
  - [--sparsity](--sparsity.md)
  - [--allow\_hf32](--allow_hf32.md)
  - [--log](--log.md)
  - [--debug\_dir](--debug_dir.md)
  - [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md)
  - [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md)
  - [--atomic\_clean\_policy](--atomic_clean_policy.md)
  - [--customize\_dtypes](--customize_dtypes.md)
  - [--is\_weight\_clip](--is_weight_clip.md)
<!-- end id13 -->

<!-- npu="910b" id12 -->
- Parameters that can be used with Atlas A2 products:
  - [--output](--output.md)
  - [--soc\_version](--soc_version.md)
  - [--precision\_mode](--precision_mode.md)
  - [--precision\_mode\_v2](--precision_mode_v2.md)
  - [--op\_select\_implmode](--op_select_implmode.md)
  - [--optypelist\_for\_implmode](--optypelist_for_implmode.md)
  - [--enable\_small\_channel](--enable_small_channel.md)
  - [--sparsity](--sparsity.md)
  - [--allow\_hf32](--allow_hf32.md)
  - [--log](--log.md)
  - [--debug\_dir](--debug_dir.md)
  - [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md)
  - [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md)
  - [--atomic\_clean\_policy](--atomic_clean_policy.md)
  - [--customize\_dtypes](--customize_dtypes.md)
  - [--is\_weight\_clip](--is_weight_clip.md)
<!-- end id12 -->

<!-- npu="310b" id9 -->
- Parameters that can be used with Atlas 200I/500 A2 inference product:
  - [--output](--output.md)
  - [--soc\_version](--soc_version.md)
  - [--precision\_mode](--precision_mode.md)
  - [--precision\_mode\_v2](--precision_mode_v2.md)
  - [--op\_select\_implmode](--op_select_implmode.md)
  - [--optypelist\_for\_implmode](--optypelist_for_implmode.md)
  - [--enable\_small\_channel](--enable_small_channel.md)
  - [--sparsity](--sparsity.md)
  - [--log](--log.md)
  - [--debug\_dir](--debug_dir.md)
  - [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md)
  - [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md)
  - [--atomic\_clean\_policy](--atomic_clean_policy.md)
  - [--customize\_dtypes](--customize_dtypes.md)
  - [--is\_weight\_clip](--is_weight_clip.md)
<!-- end id9 -->

<!-- npu="310p" id10 -->
- Parameters that can be used with Atlas inference products:
  - [--output](--output.md)
  - [--soc\_version](--soc_version.md)
  - [--core\_type](--core_type.md)
  - [--aicore\_num](--aicore_num.md)
  - [--precision\_mode](--precision_mode.md)
  - [--precision\_mode\_v2](--precision_mode_v2.md)
  - [--op\_select\_implmode](--op_select_implmode.md)
  - [--optypelist\_for\_implmode](--optypelist_for_implmode.md)
  - [--log](--log.md)
  - [--debug\_dir](--debug_dir.md)
  - [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md)
  - [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md)
  - [--atomic\_clean\_policy](--atomic_clean_policy.md)
  - [--customize\_dtypes](--customize_dtypes.md)
  - [--is\_weight\_clip](--is_weight_clip.md)
<!-- end id10 -->

<!-- npu="910" id11 -->
- Parameters that can be used with Atlas training products:
  - [--output](--output.md)
  - [--soc\_version](--soc_version.md)
  - [--precision\_mode](--precision_mode.md)
  - [--precision\_mode\_v2](--precision_mode_v2.md)
  - [--op\_select\_implmode](--op_select_implmode.md)
  - [--optypelist\_for\_implmode](--optypelist_for_implmode.md)
  - [--enable\_small\_channel](--enable_small_channel.md)
  - [--log](--log.md)
  - [--debug\_dir](--debug_dir.md)
  - [--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md)
  - [--op\_compiler\_cache\_dir](--op_compiler_cache_dir.md)
  - [--atomic\_clean\_policy](--atomic_clean_policy.md)
  - [--customize\_dtypes](--customize_dtypes.md)
  - [--is\_weight\_clip](--is_weight_clip.md)
<!-- end id11 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--singleop_res.md#id3 -->

## Parameter Values

**Parameter value:**
For details about the format and parameter configuration of the single operator description file (JSON format), see [Single Operator Model Conversion](../sinlgeop_model_convert/sinlgeop_model_convert.md).

**Parameter value constraints:**
The single operators specified by this parameter are defined based on Ascend IR. For details about the definition of single operators, see the "Ascend IR Operator Specifications" section in *[Operator Library](https://hiascend.com/document/redirect/CannCommunityOplist)*.

## Recommended Configuration and Benefits

None.

## Example

The following uses the Add single operator as an example. The description file corresponding to this single operator is _add.json_. Upload the file to any directory on the server where the ATC tool is located, for example, $HOME/singleop. The usage example is as follows:

```bash
atc --singleop=$HOME/singleop/add.json --output=$HOME/singleop/out/op_model  --soc_version=<soc_version> ...
```

## Dependencies and Constraints

- Usage Constraints

    In the scenario where a single operator JSON file is converted into an offline model, if you want the model conversion to use only TBE operators (without searching for AI CPU operators, and report an error if no TBE operator is found), you also need to set the following environment variable:

    ```bash
    export ASCEND_ENGINE_PATH=${INSTALL_DIR}/lib64/plugin/opskernel/libfe.so:${INSTALL_DIR}/lib64/plugin/opskernel/libge_local_engine.so:${INSTALL_DIR}/lib64/plugin/opskernel/librts_engine.so
    ```

    After running the preceding command, if you want to perform other operations, you need to delete the preceding environment variable: run the **unset ASCEND\_ENGINE\_PATH** command to invalidate it.

    In the command, replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, if the software is installed by the root user, the default storage path is /usr/local/Ascend/cann.

- API Constraints

    When using an application project to perform model inference on the om offline model file converted from the single operator description file, you need to call the single operator model loading API to load the operator model (for example, the [aclopSetModelDir](../../../api/graph_engine_api/c/acl/aclopSetModelDir.md) API), and finally call the single operator execution API to execute the operator (for example, the [aclopExecuteV2](../../../api/graph_engine_api/c/acl/aclopExecuteV2.md) API).
