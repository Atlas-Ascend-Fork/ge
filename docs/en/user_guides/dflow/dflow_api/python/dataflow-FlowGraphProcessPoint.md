# dataflow.FlowGraphProcessPoint

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:11:38.405Z pushedAt=2026-10-29T03:02:52.058Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Constructor of FlowGraphProcessPoint, which returns a FlowGraphProcessPoint object.

## Prototype

```python
FlowGraphProcessPoint(flow_graph, compile_config_path="", name=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| flow_graph | FlowGraph | Subgraph of dataflow.FlowGraph. |
| compile_config_path | str | Path of the configuration file used when compiling the FlowGraph. Not set by default. |
| name | str | Name of the processing point. The framework automatically ensures name uniqueness. If not set, names such as FlowGraphProcessPoint, FlowGraphProcessPoint_1, FlowGraphProcessPoint_2, ... are automatically generated. |

## Return Value

Returns None in normal scenarios.

In exceptional cases, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
pp1 = df.FlowGraphProcessPoint(...)
```

## Constraints

None
