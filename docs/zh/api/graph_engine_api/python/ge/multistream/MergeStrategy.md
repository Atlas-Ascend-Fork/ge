# MergeStrategy

流合并策略，通过`plan(merge_strategy=...)`指定。

| 枚举值 | 值 | 描述 |
| --- | --- | --- |
| `multistream.MergeStrategy.LOAD_BALANCE` | `"LoadBalance"` | 负载均衡，默认策略 |
| `multistream.MergeStrategy.MAIN_STREAM` | `"MainStream"` | 主流优先 |

相关接口：[DAGNode](DAGNode.md)、[plan](plan.md)。

## 约束说明

建议传入枚举成员；策略参数的校验规则见[plan](plan.md)。

## 调用示例

调用前需准备CANN Runtime，并完成运行时初始化或离线目标SoC配置，详见[plan的约束说明](plan.md#约束说明)。

```python
import multistream as ms

graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
result = ms.plan(graph, max_stream_num=2, merge_strategy=ms.MergeStrategy.MAIN_STREAM)
print(result.get_stream_id(node))
```
