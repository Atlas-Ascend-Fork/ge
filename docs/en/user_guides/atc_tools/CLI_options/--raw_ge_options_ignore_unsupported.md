# --raw\_ge\_options\_ignore\_unsupported

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:37:23.316Z pushedAt=2026-10-27T01:53:39.074Z -->

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

**Parameter value constraints:**
Each options key in the JSON file must be non-empty.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --raw_ge_options=$HOME/module/option.json --raw_ge_options_ignore_unsupported=false --model=model.onnx ...
```

## Dependencies and Constraints

None.
