# GEStreamAllocationSummaryIsAssignedByStreamPass

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-10-27T16:34:51.941Z pushedAt=2026-10-28T13:38:11.589Z nl=2 -->

## Product Support

See [Session API Product Support](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Feature Description

Obtains whether the root graph and subgraphs are assigned by the user-registered stream allocation Pass.

## Prototype

```c
ge::Status GEStreamAllocationSummaryIsAssignedByStreamPass(const ge::CompiledGraphSummary &compiled_graph_summary, std::map<AscendString, std::vector<bool>> &graph_to_is_assigned_by_stream_pass)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| compiled_graph_summary | Input | Summary information of the compiled graph. |
| graph_to_is_assigned_by_stream_pass | Output | Map format, where the key is the graph name and the value is the vector assigned by the stream allocation Pass, with the index representing the logical stream ID. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | - SUCCESS: The API call succeeds.<br>  - FAILED: The API call fails. |

## Constraints

None
