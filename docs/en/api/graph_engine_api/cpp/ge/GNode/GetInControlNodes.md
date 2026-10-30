# GetInControlNodes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:01:01.567Z pushedAt=2026-10-28T07:40:22.686Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/gnode.h\>
- Library file: libgraph.so

## Description

Obtains the control input nodes of the operator.

## Prototype

```c++
std::vector<GNodePtr> GetInControlNodes() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | vector\<GNodePtr> | List of control input nodes of the operator. Empty means there is no control operator. An operator may have multiple control inputs. |

## Constraints

None
