# plan

## 函数原型

```python
multistream.plan(
    graph: DAGGraph,
    *, max_stream_num: Optional[int] = None,
    base_stream_id: int = 0,
    merge_strategy: MergeStrategy = MergeStrategy.LOAD_BALANCE,
) -> StreamPlan
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| graph | 输入 | [DAGGraph](DAGGraph.md)对象，必填。 |
| max_stream_num | 输入 | 普通流数上限，整数1～64，`None`按8处理。 |
| base_stream_id | 输入 | 流ID起点，非负整数，默认0。 |
| merge_strategy | 输入 | [MergeStrategy](MergeStrategy.md)枚举，默认负载均衡。 |

## 返回值说明

返回[StreamPlan](StreamPlan.md)。失败抛出异常，不返回部分结果。

## 约束说明

- 除`graph`外均为关键字参数；流数及起点不接受bool。
- 规划不修改输入图和节点成本，也不创建Runtime流。
- 调用前需准备CANN Runtime，并完成运行时初始化或离线目标SoC配置。Runtime加载或核数查询失败时向调用方报告异常，不使用默认核数。
- 图类型或整数参数类型错误抛出`TypeError`；流数越界、起点为负或策略无效抛出`ValueError`；图有环或内部结果不一致抛出`RuntimeError`。
- 推荐使用枚举；兼容字符串`LoadBalance`、`load_balance`、`MainStream`、`main_stream`，忽略大小写，支持空格和连字符转下划线。
- 当前接口不接受`profiling_path`，不自动读取profiling CSV或选择加权策略。

## 调用示例

调用前需准备CANN Runtime，并完成运行时初始化或离线目标SoC配置，详见[plan的约束说明](plan.md#约束说明)。

```python
import multistream as ms

graph = ms.DAGGraph("example")
source = graph.add_node("source", "Relu")
target = graph.add_node("target", "Relu")
graph.add_edge(source, 0, target, 0)
result = ms.plan(graph, max_stream_num=2, merge_strategy=ms.MergeStrategy.LOAD_BALANCE)
print(result.get_required_stream_count())
print(result.get_stream_id(source), result.get_stream_id(target))
```
