# AppendNodeMatchPair

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:28:03.739Z pushedAt=2026-10-28T07:40:22.552Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/match\_result.h\>
- Library file: libge\_compiler.so

## Description

Adds a match pair.

## Prototype

```c++
Status AppendNodeMatchPair(const NodeIo &pattern_node_out,const NodeIo &matched_node_out)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pattern_node_out | Input | An output of a Node in the Pattern Graph. |
| matched_node_out | Input | The output of the corresponding matched Node in the Target Graph. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: The match pair is added successfully.<br>**FAILED**: Failed to add the match pair. |

## Constraints

None
