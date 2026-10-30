# ContextParam

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- 库文件：liblowering.so

## 功能说明

GE与自定义算子之间的运行期交互参数枚举，作为[`GetContextValue`](GetContextValue.md)接口的查询key。参数统一按数值语义取值：布尔类参数取值为0或1，数值类参数为对应数值。

## 枚举定义

```c++
enum class ContextParam : uint32_t
```

## 枚举值说明

| 枚举值 | 语义 |
| --- | --- |
| kStaticModel | 是否运行于静态图（含动态shape的静态子图），取值0或1。 |
| kAddressRefreshEnabled | 本轮执行的输出地址是否需要刷新，取值0或1。 |
| kGraphId | 当前模型所属根图的graph id。恒为根图取值，不受子图影响。 |
| kNodeId | 当前算子节点在图内的节点id，图内唯一。 |
| kVectorCoreNum | 当前算子节点可用的vector core数量。未配置时查询失败。 |
| kNum | 预留，不作为查询key使用。 |

## 约束说明

- 枚举值仅在[`Execute`](../../ge/EagerExecuteOp/Execute.md)回调内有效；新增参数只需增加枚举值，不改变接口签名。
