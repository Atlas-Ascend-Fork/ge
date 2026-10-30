# SerializeToArray

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

将任务描述序列化为字节流写入用户缓冲区，序列化结果与domi::TaskDef::SerializeToArray逐字节一致，消费方可通过ParseFromArray直接反序列化。

## 函数原型

```c++
bool SerializeToArray(uint8_t *data, size_t size) const
```

## 参数说明

| 参数 | 输入/输出 | 说明 |
| --- | --- | --- |
| data | 输出 | 序列化输出缓冲区，建议通过[ByteSizeLong](ByteSizeLong.md)分配。 |
| size | 输入 | 缓冲区长度（字节）。 |

## 返回值说明

- true：序列化成功。
- false：序列化失败（内部对象创建失败、data为空或size为0），调用方应返回任务生成失败。

## 约束说明

缓冲区长度不应小于[ByteSizeLong](ByteSizeLong.md)的返回值。

## 调用示例

```c++
// GenTask回调中：context为回调入参（gert::ExeResGenerationContext*），tasks为回调出参，
// RT_MODEL_TASK_KERNEL为算子侧局部常量0（Kernel类型任务）
gert::TaskDefBuilder task;
task.SetStreamId(context->GetStreamId()).SetType(RT_MODEL_TASK_KERNEL);
...
const size_t pbSize = task.ByteSizeLong();
std::vector<uint8_t> pbBuff(pbSize, 0);
if (!task.SerializeToArray(pbBuff.data(), pbSize)) {
  return ge::GRAPH_FAILED;
}
tasks.emplace_back(std::move(pbBuff));
```
