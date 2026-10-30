# Replace

<!-- md-trans-meta sourceCommit=aceafa20475610d02e1ce8acd4343ad8a8ed58cc translatedAt=2026-10-16T13:46:00.868Z pushedAt=2026-10-28T07:40:22.636Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/graph\_rewriter.h\>
- Library file: libge\_compiler.so

## Description

Given a **SubgraphBoundary**, replaces the operators within the boundary with **replacement**.

## Prototype

```c++
static Status Replace(const SubgraphBoundary &subgraph, const Graph &replacement)
static Status Replace(const SubgraphBoundary &subgraph, Graph &&replacement)
static Status Replace(const SubgraphBoundary &subgraph, Graph &&replacement, CustomPassContext &ctx)
static Status Replace(const SubgraphBoundary &subgraph, const Graph &replacement, CustomPassContext &ctx)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| subgraph | Input | Subgraph boundary to be replaced. |
| replacement | Input | Replacement graph. (Describes the graph structure that the subgraph is about to be replaced with.) |
| ctx | Input | Pass context. When this overload is used, fusibility check ([CanFuse](../GraphFuseInspectorUtils/CanFuse.md)) and fusion result reporting ([ReportFuse](../GraphFuseInspectorUtils/ReportFuse.md)) are performed automatically, and the fusion source is recorded through **pass_name** in **ctx**. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | Whether the replacement succeeds. |

## Constraints

None
