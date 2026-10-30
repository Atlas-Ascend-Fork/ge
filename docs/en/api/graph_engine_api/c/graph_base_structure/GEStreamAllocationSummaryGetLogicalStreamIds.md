# GEStreamAllocationSummaryGetLogicalStreamIds

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-10-27T16:32:58.350Z pushedAt=2026-10-28T13:37:08.838Z nl=2 -->

## Product Support

See [Session API product support](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Feature Description

Obtains the logical stream IDs of the root graph and subgraphs.

## Prototype

```c
ge::Status GEStreamAllocationSummaryGetLogicalStreamIds(const ge::CompiledGraphSummary &compiled_graph_summary, std::map<AscendString, std::vector<int64_t>> &graph_to_logical_stream_ids)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| compiled_graph_summary | Input | Summary information of the compiled graph. |
| graph_to_logical_stream_ids | Output | Map format, where the key is the graph name and the value is a vector of logical stream IDs, with the index being the logical stream ID. |

## Return Value

| Parameter Name | Type | Description |
| --- | --- | --- |
| - | ge::Status | - SUCCESS: The API is called successfully.<br>  - FAILED: The API call fails. |

## Constraints

None
