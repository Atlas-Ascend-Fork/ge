# GEStreamAllocationSummaryGetUsrStreamLabels

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-11-17T15:51:13.255Z pushedAt=2026-11-18T06:46:22.107Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Function Description

Obtains the user stream labels of the root graph and subgraphs.

## Prototype

```c
ge::Status GEStreamAllocationSummaryGetUsrStreamLabels(const ge::CompiledGraphSummary &compiled_graph_summary, std::map<AscendString, std::vector<AscendString>> &graph_to_user_stream_labels)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| compiled_graph_summary | Input | Summary information of the compiled graph. |
| graph_to_user_stream_labels | Output | Map format, where the key is the graph name and the value is a vector of user stream labels, with the index being the logical stream ID. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | - SUCCESS: The API call succeeds.<br>  - FAILED: The API call fails. |

## Constraints

None
