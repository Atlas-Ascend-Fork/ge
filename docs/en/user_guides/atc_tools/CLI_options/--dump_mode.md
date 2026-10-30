# --dump\_mode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T02:57:53.018Z pushedAt=2026-10-23T13:37:37.812Z -->

## Product Support

Supported on all chips.

## Function Description

Whether to generate a JSON file with shape information.

## Related Parameters

- This parameter must be used together with [--json](--json.md), [--mode](--mode.md)=1, [--framework](--framework.md), and [--om](--om.md) (which must be the original model file; if the model file is in the Caffe framework, the [--weight](--weight.md) parameter is also required).

## Parameter Values

- 0: (default) Generates a JSON file without shape information.
- 1: Generates a JSON file with shape information.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --mode=1 --om=$HOME/module/resnet50_tensorflow.pb --json=$HOME/module/out/tf_resnet50.json --framework=3 --dump_mode=1 ...
```

## Dependencies and Constraints

None.
