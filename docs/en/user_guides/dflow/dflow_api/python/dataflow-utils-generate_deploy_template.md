# dataflow.utils.generate\_deploy\_template

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:16:02.974Z pushedAt=2026-10-29T03:04:50.554Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Generates a template of the file required by option:"ge.experiment.data\_flow\_deploy\_info\_path" at the specified deployment location based on the FlowGraph.

## Prototype

```python
generate_deploy_template(graph, file_path)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| graph | FlowGraph | The constructed FlowGraph. |
| file_path | str | The path (including the file name) of the file to be generated for specifying the deployment information. |

## Return Value

None.

## Examples

```python
import dataflow as df
#Omit the graph construction process
dag = df.FlowGraph([output])
df.utils.generate_deploy_template(dag, "deploy_info.json")
```

## Constraints

The generated template uses device0 as the default deployment location. To change the location, manually modify the file.
