# --mode

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T03:18:55.387Z pushedAt=2026-10-26T04:02:39.059Z -->

## Product Support

Supported on all chips.

## Function Description

Running mode.

## Related Parameters

- If the value of [--mode](--mode.md) is 1 or 5, this parameter must be used together with [--om](--om.md) and [--json](--json.md). If the original model file is converted into a JSON file with shape information, this parameter must also be used together with [--dump\_mode](--dump_mode.md).
- If the value of [--mode](--mode.md) is 6, this parameter only needs to be used together with [--om](--om.md).
- If the value of [--mode](--mode.md) is 3 and you need to specify the save path of the pre-check result, this parameter must be used together with [--check\_report](--check_report.md).

## Parameter Values

**Parameter Value:**

- 0: (default) Generates an offline model adapted to the AI processor. The model file format is \*.om.
- 1: Converts an offline model or original model file to a JSON file for easy viewing of parameter information in the model.
- 3: Performs only a pre-check to verify whether the content of the model file is valid.
- 5: Converts a dump graph structure file to a JSON file. It is used to parse the dump graph structure generated during graph compilation (ge\_proto\*.txt format file; ge\_onnx\*.pbtxt is not supported yet), and then converts the dump graph structure to a JSON file for easy locating by users.
- 6: Displays model information for an existing **offline model**, including key resource information occupied by the model and the compilation and runtime environment information.

**Parameter Value Constraints:**

If the value of [--mode](--mode.md) is 5, you need to set the relevant environment variables to obtain the dump graph structure file first. For details, see [2.c](../overview/environment_setup.md). After the environment variables are set and the model conversion is complete, the corresponding graph structure file is generated in the current path where the atc command is executed.

## Recommended Configuration and Benefits

None.

## Examples

- The parameter value is 0:

    ```bash
    atc --mode=0 --framework=3 --model=$HOME/module/resnet50_tensorflow.pb  --output=$HOME/module/out/tf_resnet50  --soc_version=<soc_version>
    ```

- The parameter value is 1:
  - Convert an offline model to a JSON file

    ```bash
    atc --mode=1 --om=$HOME/module/out/tf_resnet50.om  --json=$HOME/module/out/tf_resnet50.json
    ```

  - Convert an original model file to a JSON file

    ```bash
    atc --mode=1 --om=$HOME/module/resnet50_tensorflow.pb  --json=$HOME/module/out/tf_resnet50.json  --framework=3
    ```

- The parameter value is 3:

    ```bash
    atc --mode=3 --framework=3 --model=$HOME/module/resnet50_tensorflow.pb --soc_version=<soc_version>
    ```

    After the execution is complete, a pre-check result file check\_result.json is generated in the current path.

- The parameter value is 5:

    ```bash
    atc --mode=5 --om=$HOME/module/ge_proto_00000000_PreRunBegin.txt --json=$HOME/module/out/ge_proto.json
    ```

- The parameter value is 6:

    ```bash
    atc --mode=6 --om=$HOME/module/out/tf_resnet50.om
    ```

    After the command is executed, the screen displays information similar to the following:

    ```console
    ============ Display Model Info start ============
    # ATC command used for model conversion
    Original Atc command line: ${INSTALL_DIR}/bin/atc.bin --model=$HOME/module/resnet50_tensorflow.pb  --framework=3 --output=$HOME/module/out/tf_resnet50 --soc_version=<soc_version> --display_model_info=1
    # ATC software version information, soc_version information, and original framework information
    system   info: atc_version[xxx], soc_version[xxx], framework_type[xxx].
    # Runtime memory usage, weight size, number of logical streams, and number of events
    resource info: memory_size[xxx B], weight_size[xxx B], stream_num[xxx], event_num[xxx].
    # Size of each partition in the offline model file, including ModelDef, weights, tbe_kernels, task_info, and so
    om       info: modeldef_size[xxx B], weight_data_size[xxx B], tbe_kernels_size[xxx B], cust_aicpu_kernel_store_size[xxx B], task_info_size[xxx B], so_store_size[xxx B].
    ============ Display Model Info end   ============
    ```

## Dependencies and Constraints

None.
