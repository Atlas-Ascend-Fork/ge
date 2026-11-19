# AddChangedResourceKey

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:28:00.084Z pushedAt=2026-10-28T07:40:22.788Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

During the inference of a write-type resource operator (such as stack push), if the resource shape changes, call this API to notify the framework.

Based on the changed resource key, the framework triggers re-inference of the corresponding read operator (such as stack pop).

## Prototype

```c++
graphStatus AddChangedResourceKey(const ge::AscendString &key)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| key | Input | Resource unique identifier. |

## Return Value

**graphStatus** type: **GRAPH\_SUCCESS**, representing success; **GRAPH\_FAILED**, representing failure.

## Constraints

None
