# --mode

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T14:13:40.673Z pushedAt=2026-11-17T11:05:26.500Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Running mode.

## Related Parameters

- If the value of [--mode](--mode.md) is 1 or 5, this parameter must be used together with [--om](--om.md) and [--json](--json.md). If the original model file is converted into a JSON file with shape information, this parameter must also be used together with [--dump\_mode](--dump_mode.md).
- If the value of [--mode](--mode.md) is 6, this parameter only needs to be used together with [--om](--om.md).
- If the value of [--mode](--mode.md) is 3 and you need to specify the save path of the pre-check result, this parameter must be used together with [--check\_report](--check_report.md).
<!-- npu="IPV350" id3 -->
- If the value of [--soc\_version](--soc_version.md) is Ascend035, this parameter can only be configured as 30.
<!-- end id3 -->

## Parameter Values

**Parameter values:**

- 0: (default) Generates an offline model adapted to the AI processor. The model file format is \*.om.
- 1: Converts an offline model or original model file to a JSON file for easy viewing of parameter information in the model.
- 3: Performs only a pre-check to verify whether the content of the model file is valid.
- 5: Converts a dump graph structure file to a JSON file. It is used to parse the dump graph structure generated during graph compilation (ge\_proto\*.txt format file; ge\_onnx\*.pbtxt is not supported yet), and then converts the dump graph structure to a JSON file for easy locating by users.
- 6: Displays model information for an existing **offline model**, including key resource information occupied by the model and the compilation and runtime environment information.
<!-- npu="IPV350" id1 -->
- 7: **This value is an experimental feature and is not supported for use in production environments. It will be released as a formal feature in a later version.** Generates an offline model adapted to the AI processor. The model file format is \*.om2. The differences between the \*.om2 model and the \*.om model are as follows:

  - \*.om2: \*.om2 uses a structured archive form based on the ZIP protocol. The model file can be directly decompressed and repackaged in both Linux and Windows environments. The model uses a directory-based organization internally, separately storing the model structure description, weight data, operator binary, and graph-level scheduling code, so that the model content has a structural basis that is checkable, executable, and analyzable. For details about the internal structure, see [om2 model internal structure](../references/internal_structure_of_om2_model.md). \*.om2 supports layered trimming capability and can adapt to different deployment scenarios:
    - Deliverable trimming: Based on whether the model has a static shape or a dynamic shape **(currently only static shapes are supported)**, the required execution dependencies and operator deliverables can be selectively packaged in \*.om2, avoiding redundant content and improving packaging efficiency and deployment flexibility.
    - Debug information trimming: The debug information in the model can be saved independently. In deployment scenarios, debug information is stripped by default to reduce the size. In the debugging phase, a model version containing complete debug information can be generated to facilitate troubleshooting.

  - \*.om: A monolithic binary file generated after the graph compiler optimizes the original model. The model structure information, operator binary, and weight data are encapsulated in the same file, and its internal organization and partition structure are not publicly disclosed. Because the model content lacks a splittable structural interface, the components in it cannot be independently checked, replaced, or analyzed.

    Compared with \*.om, \*.om2 reduces runtime model parsing operations (such as structure deserialization) and the construction of scheduling data structures during the loading phase, thereby effectively reducing the loading latency. In typical model scenarios, the loading time is significantly reduced, while in the execution phase, the performance remains comparable to that of \*.om.

- 30: Generates an offline model adapted to the AI processor. The model file format is \*.exeom, and a graph debug information file \*.dbg is also generated for graph debugging in dump and profiling. The comparison between the \*.om model and the \*.exeom model is as follows:
  - The \*.exeom file is aware of the specific hardware scheduling capability and contains the runtime data structure of the target execution platform (this data is saved in binary form in the \*.exeom file). During the model loading phase, the binary content is loaded and restored. Based on the data area address passed by the user application or the actually allocated data address, the address pointer values in the binary are refreshed, and then the binary content is directly copied to the Device, improving the model loading performance and reducing the peak memory occupancy during model loading. **In some resource-constrained scenarios, it is recommended to use the \*.exeom model file to enhance product competitiveness.**
  - The \*.om file is not aware of the specific hardware scheduling capability and contains an abstract data structure in an intermediate state. During the model loading phase, the runtime data structure is generated based on the scheduling characteristics of the specific execution platform.
<!-- end id1 -->

**Parameter value constraints:**

If the value of [--mode](--mode.md) is 5, you need to set the relevant environment variables to obtain the dump graph structure file first. For details, see [2.c](../overview/environment_setup.md). After the environment variables are set and the model conversion is complete, the corresponding graph structure file is generated in the current path where the atc command is executed.

## Recommended Configuration and Benefits

None.

## Example

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

- The parameter value is 7:

    ```bash
    atc --mode=7 --framework=5 --model=$HOME/module/resnet50.onnx  --output=$HOME/module/out/resnet50_npu --soc_version=<soc_version>
    ```

<!-- npu="IPV350" id2 -->
- The parameter value is 30:

    ```bash
    atc --mode=30 --framework=3 --model=$HOME/module/resnet50_tensorflow.pb  --output=$HOME/module/out/tf_resnet50  --soc_version=<soc_version>
    ```
<!-- end id2 -->

## Dependency Constraints

None.
