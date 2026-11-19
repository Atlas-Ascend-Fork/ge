# MergeStrategy

<!-- md-trans-meta sourceCommit=76a2946a47b1d8a6a7bedfc5a97e11334c04e83e translatedAt=2026-11-17T02:35:44.175Z pushedAt=2026-11-18T02:41:08.833Z nl=2 -->

Stream merge strategy, specified by `plan(merge_strategy=...)`.

| Enum Value | Value | Description |
| --- | --- | --- |
| `multistream.MergeStrategy.LOAD_BALANCE` | `"LoadBalance"` | Load balancing, the default strategy |
| `multistream.MergeStrategy.MAIN_STREAM` | `"MainStream"` | Main stream first |

Related APIs: [DAGNode](DAGNode.md), [plan](plan.md).

## Constraints

It is recommended to pass an enumeration member. For the validation rules of the strategy parameter, see [plan](plan.md).

## Examples

Before calling, prepare the CANN Runtime and complete runtime initialization or offline target SoC configuration. For details, see [Constraints of plan](plan.md#constraints).

```python
import multistream as ms

graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
result = ms.plan(graph, max_stream_num=2, merge_strategy=ms.MergeStrategy.MAIN_STREAM)
print(result.get_stream_id(node))
```
