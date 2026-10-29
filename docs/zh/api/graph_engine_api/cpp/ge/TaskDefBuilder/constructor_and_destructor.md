# 构造函数和析构函数

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

构造函数创建TaskDefBuilder对象并在内部创建空的任务描述（Pimpl持有），析构函数释放内部持有的任务描述对象。

## 函数原型

```c++
TaskDefBuilder()
~TaskDefBuilder()
TaskDefBuilder(const TaskDefBuilder &) = delete
TaskDefBuilder &operator=(const TaskDefBuilder &) = delete
```

## 参数说明

无

## 返回值说明

无

## 约束说明

- TaskDefBuilder不可拷贝、不可赋值。
- 内部任务描述对象在析构时统一释放，视图对象不参与释放。

## 调用示例

```c++
// GenTask回调中构造任务
gert::TaskDefBuilder task;
task.SetStreamId(context->GetStreamId()).SetType(RT_MODEL_TASK_KERNEL);
...
```
