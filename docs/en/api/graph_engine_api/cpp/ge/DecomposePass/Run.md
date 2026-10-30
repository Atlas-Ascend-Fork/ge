# Run

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:29:25.071Z pushedAt=2026-10-28T07:40:22.318Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/decompose\_pass.h\>
- Library file: libge\_compiler

## Description

Uses the op type declared during registration to find the matching node in the graph.

For each matched node, calls the **MeetRequirements** function defined by the subclass in turn to determine whether it can be replaced; obtains the replacement defined by the subclass, and replaces the matched nodes in the graph in turn.

Note: The **Run** function processes only the current graph. If subgraphs need to be processed, the **Pass** caller is responsible for doing so.

## Prototype

```c++
Status Run(GraphPtr &graph, CustomPassContext &pass_context) override
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Original graph to be modified. |
| pass_context | Input | Custom pass context. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Replacement succeeded.<br>**FAILED**: Replacement failed. |

## Constraints

None
