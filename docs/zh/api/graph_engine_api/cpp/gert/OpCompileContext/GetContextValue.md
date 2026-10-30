# GetContextValue

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe\_graph/runtime/op\_compile\_context.h\>
- 库文件：liblowering.so

## 功能说明

自定义算子在[`Compile`](../../ge/CompilableOp/Compile.md)回调中，可通过该接口查询编译期已确定的关键信息（根图graph id、节点id、可用vector core数量等），据此调整编译期行为。参数统一按数值语义取值：布尔类参数取值为0或1，数值类参数为对应数值；编译期不可确定的参数（如地址刷新使能，由运行期内存类型决定）查询返回false。

## 函数原型

```c++
bool GetContextValue(ContextParam key, uint64_t &value) const
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| key | 输入 | 交互参数的枚举key，取值见下表。 |
| value | 输出 | 查询到的参数取值。查询失败时不写出。 |

| 取值 | 编译期语义 |
| --- | --- |
| kGraphId | 当前编译子模型所属根图的graph id。 |
| kNodeId | 当前算子节点在图内的节点id，图内唯一。 |
| kVectorCoreNum | 当前算子节点可用的vector core数量（由`_op_vectorcore_num`属性配置）。未配置或非法时查询失败。 |
| kStaticModel、kAddressRefreshEnabled | 编译期不支持，查询返回false。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | bool | 查询成功返回true；key未定义、参数未配置、编译期不可确定或当前上下文不可用时返回false（不写出value）。 |

## 约束说明

- 仅在[`Compile`](../../ge/CompilableOp/Compile.md)回调内调用有效，参数反映本次编译的图信息，算子不应跨编译任务缓存；未设置的参数一律返回false，数值类的0是合法取值（如graph id为0），应通过返回值区分"取值为0"与"未配置"。
