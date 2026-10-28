# GetContextValue

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- 库文件：liblowering.so

## 功能说明

查询框架与自定义算子之间的运行期交互参数取值。

自定义算子在[`Execute`](../../ge/EagerExecuteOp/Execute.md)回调中，可通过该接口查询当前执行环境的关键信息（如是否运行于静态图、地址是否需要刷新、所属根图id等），据此自适应地调整执行行为。参数统一按数值语义取值：布尔类参数取值为0或1，数值类参数为对应数值。

## 函数原型

```c++
bool GetContextValue(ContextParam key, uint64_t &value) const
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| key | 输入 | 交互参数的枚举key，取值见下表。 |
| value | 输出 | 查询到的参数取值。查询失败时不写出。 |

**ContextParam取值说明：**

| 取值 | 语义 |
| --- | --- |
| kStaticModel | 是否运行于静态图（含动态shape的静态子图），取值0或1。 |
| kAddressRefreshEnabled | 本轮执行的输出地址是否需要刷新，取值0或1。 |
| kGraphId | 当前模型所属根图的graph id。恒为根图取值，不受子图影响。 |
| kNodeId | 当前算子节点在图内的节点id，图内唯一。 |
| kVectorCoreNum | 当前算子节点可用的vector core数量。未配置时查询失败。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | bool | 查询成功返回true；key未定义、参数未配置或当前上下文不可用时返回false（不写出value）。 |

## 约束说明

- 仅在[`Execute`](../../ge/EagerExecuteOp/Execute.md)回调内调用有效，此时上下文由框架注入。
- 参数取值反映的是本轮执行的上下文快照，算子不应缓存跨轮使用（每轮执行可能变化）。
- 数值类参数的0是合法取值（如graph id为0），算子应通过返回值区分"取值为0"与"未配置"，不能仅判断value是否为0。

## 调用示例

```c++
ge::graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
  // 查询是否运行于静态图
  uint64_t is_static_model = 0UL;
  if (ctx->GetContextValue(gert::ContextParam::kStaticModel, is_static_model) && (is_static_model == 1UL)) {
    // 静态图路径：地址固定，可跳过逐轮地址相关处理
  }

  // 查询vector core数量（未配置时查询失败，需自行处理）
  uint64_t vector_core_num = 0UL;
  if (ctx->GetContextValue(gert::ContextParam::kVectorCoreNum, vector_core_num)) {
    // 按vector core数量下发kernel
  }
  return ge::GRAPH_SUCCESS;
}
```
