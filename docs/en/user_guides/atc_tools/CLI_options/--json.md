# --json

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T03:16:25.447Z pushedAt=2026-10-26T02:55:56.323Z -->

## Product Support

<!-- npu="950,A3,910b,910,310p,310b" id2 -->
Supported on all chips
<!-- end id2 -->

<!-- @ref: ge/res/docs/zh/user_guides/atc_tools/CLI_options/--json_res.md#id1 -->

## Feature Overview

Path and file name of the JSON file converted from an offline model, an original model file, or a GE dump graph structure file.

If an existing offline model is converted to a JSON file, you can also view in the converted file the base version used when the original model was converted to the offline model (for example, the ATC software version and the OPP operator package version) and the atc command used for the model conversion at that time.

## Related Parameters

- Converting an offline model to a JSON file

    This parameter must be used together with [--mode](--mode.md)=1 and [--om](--om.md).

- Converting an original model file to a JSON file

    This parameter must be used together with [--mode](--mode.md)=1, [--om](--om.md), and [--framework](--framework.md).

    When the original model is in the MindSpore framework, that is, when --framework=1, conversion to a JSON file is not supported.

- Converting a GE dump graph structure file to a JSON file

    This parameter must be used together with [--mode](--mode.md)=5 and [--om](--om.md).

    Only the dumped ge\_proto\*.txt format file is supported for conversion to a JSON file.

## Parameter Value

**Parameter value:**
Path and file name of the JSON file.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

## Recommended Configuration and Benefits

None.

## Example

- Convert an offline model to a JSON file

    ```bash
    atc --mode=1 --om=$HOME/module/out/tf_resnet50.om  --json=$HOME/module/out/tf_resnet50.json ...
    ```

- Convert an original model file to a JSON file

    ```bash
    atc --mode=1 --om=$HOME/module/resnet50_tensorflow.pb  --json=$HOME/module/out/tf_resnet50.json  --framework=3 ...
    ```

- Convert a GE dump graph structure file to a JSON file

    ```bash
    atc --mode=5 --om=$HOME/module/ge_proto_00000000_PreRunBegin.txt --json=$HOME/module/out/ge_proto.json ...
    ```

## Dependency Constraints

None.
