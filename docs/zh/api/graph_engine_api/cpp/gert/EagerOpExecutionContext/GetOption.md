# GetOption

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- 库文件：liblowering.so

## 功能说明

获取运行期选项的取值。

选项按全局、会话、图三级合并语义查询（图级优先级最高），供自定义算子在[`Execute`](../../ge/EagerExecuteOp/Execute.md)回调中读取影响自身行为的配置项（如`ge.exec.atomicCleanPolicy`、`ge.hardwareInfo`等），无需感知选项的来源层级。

## 函数原型

```c++
bool GetOption(const ge::AscendString &option_key, ge::AscendString &option_value) const
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| option_key | 输入 | 选项key，如`ge.exec.atomicCleanPolicy`。 |
| option_value | 输出 | 查询到的选项取值。查询失败时不写出。 |

## 返回值说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| - | bool | 查询成功返回true；选项未配置或当前上下文不可用时返回false（不写出value）。 |

## 约束说明

- 仅在[`Execute`](../../ge/EagerExecuteOp/Execute.md)回调内调用有效。
- 该接口读取的是当前执行上下文可见的选项；选项在模型执行期间被修改时，是否可见取决于其配置层级，算子不应依赖跨轮的一致性。
- 选项取值为字符串语义，数值含义由选项自身约定，算子需自行解析。

## 调用示例

```c++
ge::graphStatus Execute(gert::EagerOpExecutionContext *ctx) override {
  // 读取运行期选项
  ge::AscendString option_value;
  if (ctx->GetOption(ge::AscendString("ge.exec.atomicCleanPolicy"), option_value)) {
    // 按选项取值调整执行行为
  }
  return ge::GRAPH_SUCCESS;
}
```
