# SetBlockDim

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

设置Kernel任务的多核并行度，对应KernelDef的block\_dim字段。一般取自节点hcom\_block\_dim等属性或tiling结果。

## 函数原型

```c++
KernelDefBuilder &SetBlockDim(uint32_t block_dim)
```

## 参数说明

| 参数 | 输入/输出 | 说明 |
| --- | --- | --- |
| block_dim | 输入 | 核并行度。 |

## 返回值说明

返回KernelDefBuilder对象引用，支持链式调用；返回值可忽略。

## 约束说明

无

## 调用示例

```c++
// GenTask回调中：blockDim取自节点属性（如hcom_block_dim），
// argsFormat由ge::ArgsFormatSerializer::Serialize(std::vector<ge::ArgDescInfo>)序列化得到
gert::TaskDefBuilder task;
task.GetKernelBuilder().SetBlockDim(blockDim).GetContextBuilder().SetArgsFormat(argsFormat);
```
