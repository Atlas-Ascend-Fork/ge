# GEStreamAllocationSummaryGetAllNodes

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-10-27T16:31:03.183Z pushedAt=2026-10-28T13:36:08.814Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Feature Description

Obtains all nodes of the root graph and subgraphs.

## Prototype

```c
ge::Status GEStreamAllocationSummaryGetAllNodes(const ge::CompiledGraphSummary &compiled_graph_summary, std::map<AscendString, std::vector<std::vector<GNode>>> &graph_to_all_nodes);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| compiled_graph_summary | Input | Summary information of the compiled graph. |
| graph_to_all_nodes | Output | In map format, where the key is the graph name and the value is a vector of all nodes, with the index representing the logical stream ID. |

## Return Value

| Parameter Name | Type | Description |
| --- | --- | --- |
| - | ge::Status | - SUCCESS: The API is called successfully.<br>  - FAILED: The API call fails. |

## Constraints

None
