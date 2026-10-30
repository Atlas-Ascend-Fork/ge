# --enable\_attr\_compression

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T02:59:43.656Z pushedAt=2026-10-23T13:53:18.116Z -->

## Product Support

Supported on all chips.

## Feature Description

Whether to compress the attributes in the graph.

When the ATC tool is used to generate an offline model, the attributes in the graph are automatically compressed to reduce the model size. However, in some scenarios, decompressing the compressed attributes is time-consuming and may affect inference performance. Therefore, this parameter is added to control whether to compress the graph attributes when generating an offline model.

## Parameter Values

- false: When generating an offline model, attributes in the graph are not compressed.
- true: (Default) When generating an offline model, attributes in the graph are automatically compressed.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --enable_attr_compression=true ...
```
