# GEStreamAllocationSummaryGetStreamGraphs

<!-- md-trans-meta sourceCommit=7b50d660bdfbb603f65d2334f268aca0e63d2718 translatedAt=2026-11-17T15:50:30.278Z pushedAt=2026-11-18T06:45:59.407Z nl=2 -->

## Product Support

See [Session API Applicable Products](../../cpp/ge/Session/overview.md).

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Function Description

Obtains the stream graphs of the root graph and subgraphs.

## Prototype

```c
ge::Status GEStreamAllocationSummaryGetStreamGraphs(const ge::CompiledGraphSummary &compiled_graph_summary, std::map<AscendString, AscendString> &graph_to_stream_graphs)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| compiled_graph_summary | Input | Summary information of the compiled graph. |
| graph_to_stream_graphs | Output | Map format, where the key is the graph name and the value is the stream graph string. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | ge::Status | - SUCCESS: The API call succeeds.<br>  - FAILED: The API call fails. |

## Constraints

None
