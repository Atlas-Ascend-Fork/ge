# SaveGraphsToPb

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T13:22:24.944Z pushedAt=2026-10-28T07:42:19.823Z nl=2 -->

## Applicable Products

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Not supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference products: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Supported
<!-- end id6 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

**This API is deprecated. Do not use it.**

Saves the graphs in a Session to a pb file. It supports weight sharing within a Session, meaning that if two graphs in a Session have identical weight values, only one copy of the weight is saved.

**Differences between this API and [ShardGraphsToFile](ShardGraphsToFile.md):**

This API applies to any graph, while [ShardGraphsToFile](ShardGraphsToFile.md) applies to distributed compilation splitting scenarios for large models.

## Prototype

```c++
Status SaveGraphsToPb(const char_t *file_path) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| file_path | Input | Directory for saving the graph and weights. It must be a valid path. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success<br>**FAILED**: Failure |

## Constraints

Weights within the same graph cannot be shared.
