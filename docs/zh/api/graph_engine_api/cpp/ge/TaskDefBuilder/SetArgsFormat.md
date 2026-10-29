# SetArgsFormat

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

设置任务的Args地址布局描述，对应KernelContext的args\_format字段。args\_format由[ArgsFormatSerializer::Serialize](../ArgsFormatSerializer/Serialize.md)对ArgDescInfo序列化得到。布局顺序由args\_format字符串自身定义，本接口不做校验。

## 函数原型

```c++
KernelContextBuilder &SetArgsFormat(const ge::AscendString &args_format)
```

## 参数说明

| 参数 | 输入/输出 | 说明 |
| --- | --- | --- |
| args_format | 输入 | 序列化后的Args布局描述字符串。 |

## 返回值说明

返回KernelContextBuilder对象引用，支持链式调用；返回值可忽略。

## 约束说明

无

## 调用示例

```c++
// 构造Args布局：隐藏输入（HCCL superkernel）在最前，随后为输入、输出
std::vector<ge::ArgDescInfo> argDescs;
argDescs.emplace_back(ge::ArgDescInfo::CreateHiddenInput(ge::HiddenInputSubType::kHcclSuperKernel));
argDescs.emplace_back(ge::ArgDescType::kIrInput, 0);
argDescs.emplace_back(ge::ArgDescType::kIrOutput, 0);
gert::TaskDefBuilder task;
task.GetKernelBuilder().GetContextBuilder()
    .SetArgsFormat(ge::ArgsFormatSerializer::Serialize(argDescs));
```
