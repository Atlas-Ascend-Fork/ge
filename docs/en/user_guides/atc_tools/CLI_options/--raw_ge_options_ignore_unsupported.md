# --raw\_ge\_options\_ignore\_unsupported

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-11-16T14:32:20.309Z pushedAt=2026-11-17T11:54:30.184Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Controls the behavior of the ATC tool when the JSON file pointed to by the [--raw\_ge\_options](--raw_ge_options.md) parameter contains GE options that are not supported by the ATC tool.

## Related Parameters

Takes effect only when [--raw\_ge\_options](--raw_ge_options.md) is not empty.

## Parameter Values

**Parameter values:**

- true: The ATC tool ignores unsupported options and prints a warning, and continues to process the remaining options.
- false (default): The ATC tool reports an error and terminates when it encounters an unsupported option.

**Parameter Value Constraints:**
Each options key in the JSON file must be non-empty.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --raw_ge_options=$HOME/module/option.json --raw_ge_options_ignore_unsupported=false --model=model.onnx ...
```

## Dependency Constraints

None.
