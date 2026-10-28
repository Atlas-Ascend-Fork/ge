# 自动多流并行

## 简介

### 功能介绍

Multistream是一个纯Python多流规划模块。调用者将计算任务及其依赖关系描述为有向无环图，模块为每个参与规划的节点分配流ID，并给出所需流数。执行器可据此将互不依赖的任务安排到不同流，利用计算重叠提高设备资源利用率。

该技术适合具有独立分支的计算图。对于主要由串行依赖构成的任务，增加流数不一定带来收益；最终效果还取决于计算资源、访存压力和同步开销。

### 实现原理

多流规划分为三个步骤：

1. 描述依赖：用节点表示计算任务，用边表示数据或控制依赖。
2. 生成方案：分析图中的执行路径，按指定策略合并到有限数量的流，并保留串行组约束。
3. 应用方案：调用方读取节点流ID和总流数，创建执行资源、安排任务，并为跨流依赖设置同步。

例如，两条分支都依赖同一输入、彼此没有依赖时，具备并行执行的条件；它们的汇合节点仍需等待两条分支完成。规划保持这些依赖，不改变任务的计算语义。

### 使用约束

- 输入必须是有向无环图，完整包含数据与控制依赖。
- 模块只生成方案，流创建、任务下发和跨流同步由调用方负责。流ID不是Runtime stream句柄。
- 普通流数上限不包含额外串行组，调用方应按返回的总流数预留资源。
- 当前公开入口提供负载均衡和主流优先两种策略，核数建议接口返回0，表示保留原配置。

## 使用方式

### 准备环境

搭建完整的AI处理器环境。按照[环境准备](overview/environment_setup.md)安装驱动固件和CANN软件包，并设置环境变量。

### 构图与规划

下面用一个分叉后汇合的计算图演示调用流程。运行示例前需完成上述环境准备。

示例通过[DAGGraph](../../api/graph_engine_api/python/ge/multistream/DAGGraph.md)创建图、添加节点和依赖边，调用[plan](../../api/graph_engine_api/python/ge/multistream/plan.md)生成方案，再通过返回的[StreamPlan](../../api/graph_engine_api/python/ge/multistream/StreamPlan.md)查询流分配结果。

```python
import multistream as ms

# 创建任务依赖图，节点名称在图内唯一。
graph = ms.DAGGraph("branches")
source = graph.add_node("source", "Op")
left = graph.add_node("left", "Op")
right = graph.add_node("right", "Op")
join = graph.add_node("join", "Op")

# source分叉到left和right，再汇合到join；left和right之间没有依赖。
# add_edge参数依次为源节点、源端口、目标节点、目标端口。
graph.add_edge(source, 0, left, 0)
graph.add_edge(source, 1, right, 0)
graph.add_edge(left, 0, join, 0)
graph.add_edge(right, 0, join, 1)

# 使用负载均衡策略，普通流数上限为2。
result = ms.plan(
    graph,
    max_stream_num=2,
    merge_strategy=ms.MergeStrategy.LOAD_BALANCE,
)

# 查询所需流总数及各节点的流ID，供调用方创建流和安排任务。
print("required streams:", result.get_required_stream_count())
for node in graph.get_nodes():
    print(node.get_name(), result.get_stream_id(node))
```

流数是上限，示例不保证一定分配两条流。调用方可以在规划前设置节点执行引擎类型、成本和串行标签，再通过相同入口生成方案；接口参数及约束见[多流规划接口](../../api/graph_engine_api/python/ge/multistream/multistream_interface.md)。

### 选择策略与使用结果

| 策略 | 选择方式 |
| --- | --- |
| 负载均衡 | [MergeStrategy.LOAD_BALANCE](../../api/graph_engine_api/python/ge/multistream/MergeStrategy.md) |
| 主流优先 | [MergeStrategy.MAIN_STREAM](../../api/graph_engine_api/python/ge/multistream/MergeStrategy.md) |

普通流数上限可设为1～64。可通过`base_stream_id`为本次规划指定流ID起点。

规划成功后，先读取[get_required_stream_count()](../../api/graph_engine_api/python/ge/multistream/StreamPlan.md)预留资源，再通过[get_stream_id(node)](../../api/graph_engine_api/python/ge/multistream/StreamPlan.md)建立节点到实际流的映射。被排除的普通节点返回 -1，调用方需按自己的执行语义处理，不应为 -1创建流。不同流之间有依赖时，消费者必须等待生产者完成。

如需让一组任务串行执行，可在规划前给节点设置相同的非空`stream_label`。该组使用一条额外流；标签不代替依赖边，调用方仍需完整描述任务间依赖。

## 效果验证

保持任务、输入和运行环境一致，比较使用方案前后的结果正确性和端到端耗时。先选择较小的流数上限，再比较不同策略和流数的效果。规划耗时和设备执行收益应分别度量，不能仅凭流数增加判断性能提升。
