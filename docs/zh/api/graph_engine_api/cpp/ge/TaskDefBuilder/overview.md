# 简介

gert::TaskDefBuilder用于在GenTask回调中构造与domi::TaskDef线格式逐字节兼容的任务描述，并序列化为字节流，供下游任务生成流程直接反序列化消费。采用Pimpl层级视图封装隔离task.proto：TaskDefBuilder对应TaskDef消息（根，独占所有权），KernelDefBuilder对应KernelDef消息（视图），KernelContextBuilder对应KernelContext消息（视图）。

层级视图与proto消息树一一对应，任务字段的写入路径与消息树路径一致，例如`task.GetKernelBuilder().SetBlockDim(x).GetContextBuilder().SetArgsFormat(s)`对应`TaskDef.kernel.context.args_format`。

## 需要包含的头文件

```c++
#include <exe_graph/runtime/task_def_builder.h>
```

## TaskDefBuilder公开接口

```c++
TaskDefBuilder()
~TaskDefBuilder()
TaskDefBuilder &SetStreamId(uint32_t stream_id)
TaskDefBuilder &SetType(uint32_t type)
KernelDefBuilder GetKernelBuilder() const
size_t ByteSizeLong() const
bool SerializeToArray(uint8_t *data, size_t size) const
```

## KernelDefBuilder公开接口（视图）

```c++
KernelDefBuilder &SetBlockDim(uint32_t block_dim)
KernelContextBuilder GetContextBuilder() const
```

## KernelContextBuilder公开接口（视图）

```c++
KernelContextBuilder &SetArgsFormat(const ge::AscendString &args_format)
```

## 约束说明

- TaskDefBuilder不可拷贝、不可赋值（独占内部proto对象所有权，析构时释放）。
- 视图对象（KernelDefBuilder/KernelContextBuilder）不拥有子消息，生命周期必须被父TaskDefBuilder覆盖；视图只能通过GetKernelBuilder()/GetContextBuilder()获得，不可凭空构造。
- 链式setter返回引用，返回值可忽略。
- 该组接口自CANN 9.3.0版本起对外提供。
