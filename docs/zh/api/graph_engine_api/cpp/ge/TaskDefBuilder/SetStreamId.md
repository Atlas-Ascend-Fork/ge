# SetStreamId

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

设置任务的流ID，对应TaskDef的stream\_id字段。

## 函数原型

```c++
TaskDefBuilder &SetStreamId(uint32_t stream_id)
```

## 参数说明

| 参数 | 输入/输出 | 说明 |
| --- | --- | --- |
| stream_id | 输入 | 任务执行的流ID，一般取自ExeResGenerationContext::GetStreamId()。 |

## 返回值说明

返回TaskDefBuilder对象引用，支持链式调用；返回值可忽略。

## 约束说明

无

## 调用示例

```c++
// GenTask回调中：context为回调入参（gert::ExeResGenerationContext*），
// RT_MODEL_TASK_KERNEL为算子侧局部常量0（Kernel类型任务）
gert::TaskDefBuilder task;
task.SetStreamId(context->GetStreamId()).SetType(RT_MODEL_TASK_KERNEL);
```
