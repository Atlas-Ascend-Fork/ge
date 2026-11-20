# CreateVariable

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:04:50.830Z pushedAt=2026-10-28T07:40:22.460Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header/Library Files

- Header file: \#include <ge/es\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Creates a variable node.

## Prototype

```c++
EsTensorHolder CreateVariable(int32_t index, const char *name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| index | Input | Variable node index. |
| name | Input | Variable node name. Variables with the same name are shared, so it must be explicitly specified. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsTensorHolder | Returns the created variable tensor holder, or an invalid **EsTensorHolder** on failure. |

## Constraints

None

## Examples

```c++
EsGraphBuilder builder("test_graph");
auto v1 = builder.CreateVariable(0, "var0");
```
