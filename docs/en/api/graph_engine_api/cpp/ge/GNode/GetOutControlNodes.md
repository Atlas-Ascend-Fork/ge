# GetOutControlNodes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:04:22.575Z pushedAt=2026-10-28T07:40:22.698Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the control output nodes of the operator.

## Prototype

```c++
std::vector<GNodePtr> GetOutControlNodes() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | vector\<GNodePtr> | List of control output nodes of the operator. Empty indicates that there is no control operator. |

## Constraints

None
