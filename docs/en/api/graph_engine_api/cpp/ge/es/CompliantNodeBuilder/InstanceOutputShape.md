# InstanceOutputShape

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:35:14.316Z pushedAt=2026-10-28T07:40:22.340Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- **Header file**: \#include <ge/compliant\_node\_builder.h\>
- **Library file**: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Sets the output shape.

## Prototype

```c++
CompliantNodeBuilder &InstanceOutputShape(const char_t *name, const std::vector<int64_t> &shape)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output name. |
| shape | Input | Shape vector. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | CompliantNodeBuilder & | Reference to the current builder object, supporting chained calls. |

## Constraints

None
