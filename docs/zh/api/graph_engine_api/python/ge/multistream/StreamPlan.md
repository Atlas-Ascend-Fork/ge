# StreamPlan

## 函数原型

```python
get_stream_id(node: DAGNode) -> int
get_required_stream_count() -> int
get_core_num(node: DAGNode, kernel_type: str) -> int
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| node | 输入 | 本次规划原图中的[DAGNode](DAGNode.md)。 |
| kernel_type | 输入 | 资源类型，只接受字符串`"AIC"`或`"AIV"`。 |

## 返回值说明

| 方法 | 返回值 |
| --- | --- |
| `get_stream_id` | 节点流ID；被排除的普通节点返回`multistream.INVALID_STREAM_ID`（-1）。 |
| `get_required_stream_count` | 普通流与额外串行流的总数，不包含起点偏移。 |
| `get_core_num` | 当前版本返回0，表示调用方保留原核数配置。 |

## 约束说明

- 结果由[plan](plan.md)生成，后续规划不覆盖已有结果。
- 节点或资源类型参数的类型不合法时抛出`TypeError`；节点不属于原图或资源字符串不是AIC/AIV时抛出`ValueError`。
- 普通流ID位于`[base_stream_id, base_stream_id + max_stream_num)`；额外串行流可使总流数超过普通流数上限。
- 流ID是规划标识，不是Runtime stream句柄；调用方按总流数预留资源。

## 调用示例

调用前需准备CANN Runtime，并完成运行时初始化或离线目标SoC配置，详见[plan的约束说明](plan.md#约束说明)。

```python
import multistream as ms

graph = ms.DAGGraph("example")
node = graph.add_node("compute", "Relu")
result = ms.plan(graph, max_stream_num=2)
print(result.get_required_stream_count())
print(result.get_stream_id(node))
assert result.get_core_num(node, "AIV") == 0
```
