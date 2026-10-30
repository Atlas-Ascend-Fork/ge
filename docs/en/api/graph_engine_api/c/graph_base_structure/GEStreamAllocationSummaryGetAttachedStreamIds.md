# GEStreamAllocationSummaryGetAttachedStreamIds

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-10-27T16:32:07.192Z pushedAt=2026-10-28T13:36:43.031Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Feature Description

Obtains the logical slave stream IDs attached to the root graph and subgraphs.

## Prototype

```c
ge::Status GEStreamAllocationSummaryGetAttachedStreamIds(const ge::CompiledGraphSummary &compiled_graph_summary, std::map<AscendString, std::vector<std::vector<int64_t>>> &graph_to_attached_stream_ids)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| compiled_graph_summary | Input | Compiled graph summary. |
| graph_to_attached_stream_ids | Output | Map format, where the key is the graph name and the value is a vector of attached logical slave stream IDs, with the index representing the logical stream ID. |

## Return Value

| Parameter Name | Type | Description |
| --- | --- | --- |
| - | ge::Status | - SUCCESS: The API is called successfully.<br>  - FAILED: The API call fails. |

## Constraints

None
