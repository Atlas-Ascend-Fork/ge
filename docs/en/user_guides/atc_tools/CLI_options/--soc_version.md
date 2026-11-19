# --soc\_version

<!-- md-trans-meta sourceCommit=9f2ea985eb7ee6374346cc894560d663152ee14c translatedAt=2026-11-16T14:38:22.214Z pushedAt=2026-11-17T11:56:29.768Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Configures the AI processor model used in the inference running phase of the model.

The model conversion phase does not depend on the AI processor, but the --soc\_version value used during conversion must be the AI processor model used in the running phase of the converted model.

Compatibility notes:

<!-- npu="950,A3,910b,910,310p,310b" id11 -->
- **For 8.5.0 and later versions**: When using the ATC tool for model conversion, you must install the ops operator package that matches the target AI processor. Otherwise, compilation fails.
<!-- end id11 -->
- For different products, as long as the --soc\_version value is the same, you only need to convert the model once, and then it can be deployed and run on these products separately.
- Deployment environment requirements: Ensure that the CANN software package version of the product running environment is not lower than the CANN software package version of the conversion environment used to convert the offline om model.
- An offline om model converted in a lower-version CANN software package environment can run in a higher-version CANN software package environment, with compatibility across four version cycles.

## Related Parameters

None.

<!-- npu="950,A3,910b,910,310p,310b" id5 -->
## Parameter Values

The method for querying the parameter value is as follows:

<!-- npu="950" id9 -->
- For Ascend 950PR&950DT products, run the `npu-smi info -t board -i id` command on the server where the AI processor is installed to query the Chip Name and NPU Name information. The actual configuration value is `Chip Name_NPU Name`. For example, if the Chip Name value is `Ascendxxx` and the NPU Name value is `1234`, the actual configuration value is `Ascendxxx_1234`.

    Here, id is the device ID. The NPU ID obtained by running the `npu-smi info -l` command is the device ID.
<!-- end id9 -->

<!-- npu="A3" id8 -->
- For Atlas A3 products, run the `npu-smi info -t board -i id -c chip_id` command on the server where the AI processor is installed to query the Chip Name and NPU Name information. The actual configuration value is Chip Name_NPU Name. For example, if the Chip Name value is `Ascendxxx` and the NPU Name value is `1234`, the actual configuration value is `Ascendxxx_1234`. Where:

  - id: device ID. The NPU ID obtained by running the `npu-smi info -l` command is the device ID.

  - chip\_id: chip ID. The Chip ID obtained by running the `npu-smi info -m` command is the chip ID.
<!-- end id8 -->

<!-- npu="910b,910,310p,310b" id6 -->
- For the following products: run the `npu-smi info` command on the server where the AI processor is installed to query the Name information. The actual configuration value is `AscendName`. For example, if the Name value is `xxxyy`, the actual configuration value is `Ascendxxxyy`.

    <!-- npu="910b" id1 -->
    Atlas A2 products
    <!-- end id1 -->

    <!-- npu="310b" id2 -->
    Atlas 200I/500 A2 inference product
    <!-- end id2 -->

    <!-- npu="310p" id3 -->
    Atlas inference products
    <!-- end id3 -->

    <!-- npu="910" id4 -->
    Atlas training products
    <!-- end id4 -->
<!-- end id6 -->

<!-- end id5 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--soc_version_res.md#id1 -->

<!-- npu="IPV350" id10 -->
## Parameter Values

Ascend035
<!-- end id10 -->

## Recommended Configuration and Benefits

None.

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--soc_version_res.md#id2 -->

<!-- npu="IPV350" id7 -->
## Example

IPV350 usage example:

```bash
atc --soc_version=Ascend035 ...
```
<!-- end id7 -->

## Dependency Constraints

None.
