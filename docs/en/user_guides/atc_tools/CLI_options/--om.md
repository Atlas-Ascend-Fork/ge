# --om

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T14:15:24.115Z pushedAt=2026-11-17T11:19:25.540Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Path and file name of the offline model (.om), original model file (for example, .prototxt of the Caffe framework, .pb of the TensorFlow framework, etc.), and GE dump graph structure file (.txt).

## Related Parameters

- If the value of [--mode](--mode.md) is 1:
  - Convert an offline model to a JSON file

    --om must be used together with [--mode](--mode.md)=1 and [--json](--json.md).

  - Convert an original model file to a JSON file

    --om must be used together with [--mode](--mode.md)=1, [--json](--json.md), and [--framework](--framework.md).

- If the value of [--mode](--mode.md) is 5:

    To convert a GE dump graph structure file to a JSON file, --om must be used together with [--mode](--mode.md)=5 and [--json](--json.md).

- If the value of [--mode](--mode.md) is 6:

    To display information such as model information for an existing **offline model**, --om only needs to be used together with [--mode](--mode.md).

- When [--mode](--mode.md) is set to 7, the --om parameter is not supported.

## Parameter Values

**Parameter value:**
Path of an offline model (.om), an original model file (for example, .prototxt of the Caffe framework or .pb of the TensorFlow framework), or a GE dump graph structure file (.txt).

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

## Recommended Configuration and Benefits

None.

## Example

- If the value of [--mode](--mode.md) is 1
  - Convert an offline model to a JSON file

    ```bash
    atc --mode=1 --om=$HOME/module/out/tf_resnet50.om  --json=$HOME/module/out/tf_resnet50.json ...
    ```

  - Convert an original model file to a JSON file

    ```bash
    atc --mode=1 --om=$HOME/module/resnet50_tensorflow.pb  --json=$HOME/module/out/tf_resnet50.json --framework=3 ...
    ```

- If the value of [--mode](--mode.md) is 5

    ```bash
    atc --mode=5 --om=$HOME/module/ge_proto_00000000_PreRunBegin.txt --json=$HOME/module/out/ge_proto.json ...
    ```

- If the value of [--mode](--mode.md) is 6

    ```bash
    atc --mode=6 --om=$HOME/module/out/tf_resnet50.om ...
    ```

## Dependency Constraints

None.
