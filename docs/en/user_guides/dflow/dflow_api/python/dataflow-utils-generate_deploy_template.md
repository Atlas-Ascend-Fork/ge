# dataflow.utils.generate\_deploy\_template

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:20:15.456Z pushedAt=2026-11-16T07:52:40.942Z nl=2 -->

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
#Omit the composition process
dag = df.FlowGraph([output])
df.utils.generate_deploy_template(dag, "deploy_info.json")
```

## Constraints

The generated template uses device0 as the default deployment location. To change the location, manually modify the file.
