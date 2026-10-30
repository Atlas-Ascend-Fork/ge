# RegisterReliedOnResourceKey

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:32:48.362Z pushedAt=2026-10-28T07:40:22.804Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Registers the relied-on resource.

This API is generally called by read-type operators, such as stack pop. Because the shape of a read-type operator depends on the resource operator, this API is called to register the relied-on resource identifier.

If the shape of the resource operator changes, re-derivation of the read-type operator can be triggered.

## Prototype

```c++
graphStatus RegisterReliedOnResourceKey(const ge::AscendString &key)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| key | Input | Unique identifier of the resource. |

## Return Value

**graphStatus** type: **GRAPH\_SUCCESS**, indicating success; **GRAPH\_FAILED**, indicating failure.

## Constraints

None
