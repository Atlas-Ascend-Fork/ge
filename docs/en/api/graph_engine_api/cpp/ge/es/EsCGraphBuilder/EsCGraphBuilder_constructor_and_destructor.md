# EsCGraphBuilder Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:53:21.433Z pushedAt=2026-10-28T07:40:22.420Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

**EsCGraphBuilder** constructor and destructor.

## Prototype

```c++
EsCGraphBuilder()
explicit EsCGraphBuilder(const char *name)
~EsCGraphBuilder()
EsCGraphBuilder(EsCGraphBuilder &&) noexcept
EsCGraphBuilder &operator=(EsCGraphBuilder &&) noexcept
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Graph name. Constructs the graph builder with the specified name. |

## Return Value

None

## Constraints

None
