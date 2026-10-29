# GetContextBuilder

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe_graph/runtime/task\_def\_builder.h\>
- 库文件：libregister.so

## 功能说明

获取KernelContext消息的视图对象（[KernelContextBuilder](overview.md)），用于设置KernelContext层级的字段。内部子消息惰性创建，保证视图指向的对象非空。

## 函数原型

```c++
KernelContextBuilder GetContextBuilder() const
```

## 参数说明

无

## 返回值说明

返回KernelContextBuilder视图对象（按值，持子消息裸指针，不拥有所有权）。

## 约束说明

视图生命周期必须被父TaskDefBuilder对象覆盖，不可在父对象析构后继续使用视图。

## 调用示例

```c++
// GenTask回调中：blockDim取自节点属性（如hcom_block_dim），
// argsFormat由ge::ArgsFormatSerializer::Serialize(std::vector<ge::ArgDescInfo>)序列化得到
gert::TaskDefBuilder task;
task.GetKernelBuilder().SetBlockDim(blockDim).GetContextBuilder().SetArgsFormat(argsFormat);
```
