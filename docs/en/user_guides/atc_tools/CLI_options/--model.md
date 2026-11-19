# --model

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-11-16T14:13:06.481Z pushedAt=2026-11-17T11:05:30.215Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Path and name of the original network model file.

## Related Parameters

When the original model is a Caffe model, this parameter must be used together with [--weight](--weight.md).

## Parameter Values

**Parameter Value:**
Path and file name of the model file.

**Parameter value format:**
Path and file name: supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --model=$HOME/module/resnet50_tensorflow.pb ...
```

## Dependency Constraints

None.
