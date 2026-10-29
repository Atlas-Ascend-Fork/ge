# SetType

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

设置任务类型，对应TaskDef的type字段。Kernel类型任务（对应RT\_MODEL\_TASK\_KERNEL）传0。

## 函数原型

```c++
TaskDefBuilder &SetType(uint32_t type)
```

## 参数说明

| 参数 | 输入/输出 | 说明 |
| --- | --- | --- |
| type | 输入 | 任务类型。构造Kernel类型任务时取值0。 |

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
