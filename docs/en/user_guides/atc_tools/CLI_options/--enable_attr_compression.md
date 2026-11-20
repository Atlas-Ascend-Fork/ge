# --enable\_attr\_compression

<!-- md-trans-meta sourceCommit=becb235792507fcdd0382168301e8d0fb82e3246 translatedAt=2026-11-16T13:50:46.549Z pushedAt=2026-11-17T08:21:01.674Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Whether to compress the attributes in the graph.

When the ATC tool is used to generate an offline model, the attributes in the graph are automatically compressed to reduce the model size. However, in some scenarios, decompressing the compressed attributes is time-consuming and may affect inference performance. Therefore, this parameter is added to control whether to compress the graph attributes when generating an offline model.

## Related Parameters

When the value of [--mode](--mode.md) is 7, the attribute compression feature is not supported.

## Parameter Values

- false: When generating an offline model, attributes in the graph are not compressed.
- true: (Default) When generating an offline model, attributes in the graph are automatically compressed.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --enable_attr_compression=true ...
```
