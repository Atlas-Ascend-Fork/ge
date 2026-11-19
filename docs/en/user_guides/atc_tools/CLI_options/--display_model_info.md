# --display\_model\_info

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T13:48:09.557Z pushedAt=2026-11-17T08:12:30.063Z nl=2 -->

## Product Support

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
Supported on all chips.
<!-- end id2 -->

<!-- npu="IPV350" id1 -->
IPV350: not supported
<!-- end id1 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--display_model_info_res.md#id1 -->

## Function Description

When compiling a model from an original framework network, this option queries the key resource information occupied by the model, as well as the compilation and runtime environment information, and prints the queried information directly on the screen.

## Related Parameters

When the value of [--mode](--mode.md) is 7, this parameter cannot be configured.

## Parameter Values

**Parameter values:**

- 0: (default) Disables the query function.
- 1: Enables the query function.

**Parameter value constraints:**
This parameter does not support viewing information when a single-operator description file is converted to an offline model, that is, it cannot be used together with the [--singleop](--singleop.md) parameter.

## Recommended Configuration and Benefits

None.

## Example

The following example uses a TensorFlow framework network model for illustration:

```bash
atc --model=$HOME/module/resnet50_tensorflow.pb  --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version> --display_model_info=1 ...
```

After the command is executed, the screen displays information similar to the following:

```console
============ Display Model Info start ============
# atc command used for model conversion
Original Atc command line: ${INSTALL_DIR}/bin/atc.bin --model=$HOME/module/resnet50_tensorflow.pb  --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version> --display_model_info=1
# ATC software version information, soc_version information, and original framework information
system   info: atc_version[xxx], soc_version[xxx], framework_type[xxx].
# Runtime memory occupancy, weight size, number of logical streams, and number of events
resource info: memory_size[xxx B], weight_size[xxx B], stream_num[xxx], event_num[xxx].
# Size of each partition in the offline model file, including ModelDef, weight, tbe_kernels, task_info, and so occupancy
om       info: modeldef_size[xxx B], weight_data_size[xxx B], tbe_kernels_size[xxx B], cust_aicpu_kernel_store_size[xxx B], task_info_size[xxx B], so_store_size[xxx B].
============ Display Model Info end   ============
```

## Dependency Constraints

None.
