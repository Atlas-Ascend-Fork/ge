# --dump\_mode

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T13:49:03.200Z pushedAt=2026-11-17T08:12:35.979Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Whether to generate a JSON file with shape information.

## Related Parameters

- This parameter must be used together with [--json](--json.md), [--mode](--mode.md)=1, [--framework](--framework.md), and [--om](--om.md) (which must be the original model file; if the model file is in the Caffe framework, the [--weight](--weight.md) parameter is also required).
- When [--mode](--mode.md) is set to 7, generating a JSON file with shape information is not supported.

## Parameter Values

- 0: (default) Generates a JSON file without shape information.
- 1: Generates a JSON file with shape information.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --mode=1 --om=$HOME/module/resnet50_tensorflow.pb --json=$HOME/module/out/tf_resnet50.json --framework=3 --dump_mode=1 ...
```

## Dependency Constraints

None.
