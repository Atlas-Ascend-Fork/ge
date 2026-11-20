# CaptureTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:31:56.140Z pushedAt=2026-10-28T07:40:22.570Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern.h\>
- Library file: libge\_compiler.so

## Description

Captures a Tensor in a Pattern so that it can be obtained in order in [MatchResult](../MatchResult/MatchResult.md).

Captures the output of a Node for later quick retrieval of the Node output in MatchResult by the index used at capture time. Users can obtain the Tensor description from Match Result for verification.

## Prototype

```c++
Pattern &CaptureTensor(const NodeIo &node_output)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node_output | Input | Source of the Tensor, that is, an output of a Node.<br>Because **node_output** can uniquely identify a Tensor in a graph, **node_output** is commonly used to refer to a Tensor. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Pattern & | Reference to Pattern for convenient chained calls. |

## Constraints

None
