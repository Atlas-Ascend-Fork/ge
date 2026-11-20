# Run

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:34:32.324Z pushedAt=2026-10-28T07:40:22.586Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/pattern\_fusion\_pass.h\>
- Library file: libge\_compiler.so

## Description

Main function for executing the pass.

This function obtains the pattern graph constructed by **PatternGraphs** to construct the pattern.

It matches the target **Graph** one by one, calls the **Requires** function to verify whether the match result can be replaced, and if **true**, performs further replacement. It obtains the target structure for replacement by calling **Replacement**, and replaces the subgraph structure corresponding to the match result with the replacement subgraph.

Note: The **Run** function only processes the current graph. If subgraphs need to be processed, the pass caller is responsible for that.

## Prototype

```c++
Status Run(GraphPtr &graph, CustomPassContext &pass_context) override
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Target graph. |
| pass_context | Input | Context, which can be used to pass information such as error messages. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Execution succeeded.<br>**FAILED**: Execution failed. |

## Constraints

None
