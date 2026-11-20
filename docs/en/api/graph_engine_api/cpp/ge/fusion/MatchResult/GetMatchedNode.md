# GetMatchedNode

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:28:03.880Z pushedAt=2026-10-28T07:40:22.553Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/match\_result.h\>
- Library file: libge\_compiler.so

## Description

Obtains the matched Target Node based on the Pattern Node.

## Prototype

```c++
Status GetMatchedNode(const GNode &pattern_node, GNode &matched_node) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pattern_node | Input | A Node in the Pattern Graph. |
| matched_node | Output | The matched Node. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Matching succeeded.<br>**FAILED**: Matching failed. |

## Constraints

None
