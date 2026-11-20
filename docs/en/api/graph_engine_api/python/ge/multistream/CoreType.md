# CoreType

<!-- md-trans-meta sourceCommit=76a2946a47b1d8a6a7bedfc5a97e11334c04e83e translatedAt=2026-11-17T02:34:33.336Z pushedAt=2026-11-18T02:41:08.826Z nl=2 -->

Execution engine type of a node, set through `DAGNode.set_core_type()`.

| Enum Value | Value | Description |
| --- | --- | --- |
| `multistream.CoreType.AIC` | `"AIC"` | Cube computation |
| `multistream.CoreType.MIX_AIC` | `"MIX_AIC"` | Mixed computation primarily based on AIC |
| `multistream.CoreType.AIV` | `"AIV"` | Vector computation |
| `multistream.CoreType.MIX_AIV` | `"MIX_AIV"` | Mixed computation primarily based on AIV |
| `multistream.CoreType.UNKNOWN` | `"UNKNOWN"` | Unknown type, the default value of a node |

Related APIs: [DAGNode](DAGNode.md), [plan](plan.md).

## Constraints

When setting the execution engine type, pass an enumeration member instead of a string.

## Examples

```python
import multistream as ms

graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
node.set_core_type(ms.CoreType.AIV)
assert node.get_core_type() == ms.CoreType.AIV
```
