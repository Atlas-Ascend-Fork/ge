# GEStreamAllocationSummaryGetHcclFollowedStreamNums

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-11-17T15:49:07.169Z pushedAt=2026-11-18T06:45:48.725Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Function Description

Obtains the number of physical streams of the actual communication follower streams of the root graph and subgraphs.

## Prototype

```c
ge::Status GEStreamAllocationSummaryGetHcclFollowedStreamNums(const ge::CompiledGraphSummary &compiled_graph_summary, std::map<AscendString, std::vector<int64_t>> &graph_to_hccl_followed_stream_nums);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| compiled_graph_summary | Input | Summary information of the compiled graph. |
| graph_to_hccl_followed_stream_nums | Output | Map whose key is the graph name and whose value is a vector of the physical stream counts of the actual communication follower streams, where the index indicates the logical stream ID. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | - SUCCESS: The API call succeeds.<br>  - FAILED: The API call fails. |

## Constraints

None
