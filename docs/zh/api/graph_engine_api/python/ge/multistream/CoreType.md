# CoreType

节点执行引擎类型，通过`DAGNode.set_core_type()`设置。

| 枚举值 | 值 | 描述 |
| --- | --- | --- |
| `multistream.CoreType.AIC` | `"AIC"` | Cube计算 |
| `multistream.CoreType.MIX_AIC` | `"MIX_AIC"` | 以AIC为主的混合计算 |
| `multistream.CoreType.AIV` | `"AIV"` | Vector计算 |
| `multistream.CoreType.MIX_AIV` | `"MIX_AIV"` | 以AIV为主的混合计算 |
| `multistream.CoreType.UNKNOWN` | `"UNKNOWN"` | 未知类型，节点默认值 |

相关接口：[DAGNode](DAGNode.md)、[plan](plan.md)。

## 约束说明

设置执行引擎类型时传入枚举成员，不传入字符串。

## 调用示例

```python
import multistream as ms

graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
node.set_core_type(ms.CoreType.AIV)
assert node.get_core_type() == ms.CoreType.AIV
```
