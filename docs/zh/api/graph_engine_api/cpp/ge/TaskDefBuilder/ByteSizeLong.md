# ByteSizeLong

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

获取当前任务描述序列化后的字节长度，用于分配序列化缓冲区。与domi::TaskDef::ByteSizeLong结果一致。

## 函数原型

```c++
size_t ByteSizeLong() const
```

## 参数说明

无

## 返回值说明

返回序列化字节长度；内部对象创建失败时返回0。

## 约束说明

无

## 调用示例

```c++
// GenTask回调中构造任务
gert::TaskDefBuilder task;
...
std::vector<uint8_t> buf(task.ByteSizeLong(), 0);
task.SerializeToArray(buf.data(), buf.size());
```
