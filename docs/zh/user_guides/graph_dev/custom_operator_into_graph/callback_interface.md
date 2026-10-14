# 回调接口详解

## 接口总览

| 接口 | 方法 | 触发时机 | 必选 |
| --- | --- | --- | --- |
| EagerExecuteOp | Execute | 运行时执行kernel | 所有场景必选 |
| ShapeInferOp | InferShape/InferDataType | 编译期Shape/DataType推导 | 可选 |
| CompilableOp | Compile | 编译期RTC编译kernel | 场景2/场景3 |
| PortableOp | Serialize / Deserialize | ATC写OM/OM加载恢复 | 场景3 |
| ArgsUpdater | UpdateHostArgs | I/O地址变化时刷新 | 静态图下沉 |

## EagerExecuteOp::Execute

运行时执行回调，是最核心的接口，**所有场景必须实现**。

```c++
graphStatus Execute(gert::EagerOpExecutionContext *ctx) override;
```

**上下文API：**

| 方法 | 用途 |
| --- | --- |
| ctx->GetInputTensor(index) | 获取输入Tensor（含addr、shape、dtype、format） |
| ctx->MallocOutputTensor(index, shape, format, dtype) | 分配输出Tensor device内存 |
| ctx->MallocWorkSpace(size) | 分配workspace device内存 |
| ctx->GetStream() | 获取执行流 |
| ctx->GetOutputTensor(index) | 获取已分配的输出Tensor |
| ctx->MakeOutputRefInput(out_idx, in_idx) | 输出引用输入地址（零拷贝） |

**典型流程：**

```c++
graphStatus Execute(gert::EagerOpExecutionContext *ctx) {
  // 获取第0个输入Tensor（const，不可修改）
  auto *x = ctx->GetInputTensor(0);
  // 分配输出Tensor内存，shape/format/dtype继承自输入
  auto *y = ctx->MallocOutputTensor(0, x->GetShape(), x->GetFormat(), x->GetDataType());
  // 获取执行流，kernel在此stream上异步执行
  void *stream = ctx->GetStream();
  // 启动用户自定义kernel
  LaunchMyKernel(x->GetAddr(), y->GetAddr(), stream);
  return GRAPH_SUCCESS;
}
```

**注意事项：**

- 输出内存由context管理生命周期，调用者无需释放。
- GetInputTensor返回const Tensor\*，不可修改输入数据。
- 对于预编译kernel binary场景，需在Execute中加载binary并通过aclrtLaunchKernelWithHostArgs launch。
- **launch 参数注意元素数而非字节数**：元素个数用 GetShapeSize\(\)，GetSize\(\) 返回的是字节大小（详情请参见[快速开始](quick_start.md)）。

## ShapeInferOp::InferShape/InferDataType

编译期Shape和DataType推导回调。

```c++
graphStatus InferShape(gert::InferShapeContext *ctx) override;
graphStatus InferDataType(gert::InferDataTypeContext *ctx) override;
```

**InferShapeContext API：**

| 方法 | 用途 |
| --- | --- |
| ctx->GetInputShape(index) | 获取输入origin Shape（const Shape*） |
| ctx->GetOutputShape(index) | 获取可修改的输出Shape（Shape*） |
| ctx->GetInputTensor(index) | 获取输入Tensor（可访问数据） |

**InferDataTypeContext API：**

| 方法 | 用途 |
| --- | --- |
| ctx->GetInputDataType(index) | 获取输入DataType |
| ctx->SetOutputDataType(index, dtype) | 设置输出DataType |

**典型流程：**

```c++
// Shape推导：将输出shape设置为与输入shape相同
graphStatus InferShape(gert::InferShapeContext *ctx) {
  auto *in_shape = ctx->GetInputShape(0);
  auto *out_shape = ctx->GetOutputShape(0);
  *out_shape = *in_shape;
  return GRAPH_SUCCESS;
}

// DataType推导：将输出类型设置为与输入类型相同
graphStatus InferDataType(gert::InferDataTypeContext *ctx) {
  return ctx->SetOutputDataType(0, ctx->GetInputDataType(0));
}
```

广播语义示例请参见[广播语义示例](concept_and_design.md#广播语义示例)。

## CompilableOp::Compile

在线编译回调，在GE/ATC编译阶段被调用。

```c++
graphStatus Compile(gert::OpCompileContext *ctx) override;
```

**OpCompileContext API：**

| 方法 | 用途 |
| --- | --- |
| ctx->GetInputTensor(index) | 获取编译期输入Tensor（含shape、dtype、format） |
| ctx->GetOutputTensor(index) | 获取编译期输出Tensor |
| ctx->GetOption(key, value) | 获取编译选项 |
| ctx->GetPlatformInfos(...) | 获取目标平台信息（芯片型号等） |

**典型流程：**

```c++
graphStatus Compile(gert::OpCompileContext *ctx) {
  // 获取编译期输入Tensor（含shape、dtype、format等元信息）
  auto *input = ctx->GetInputTensor(0);
  // 根据输入shape生成binary缓存key，支持多shape场景下的kernel管理
  auto key = BuildBinaryKey(input->GetShape());
 // 加载kernel源码文件
  auto source = LoadFile("my_kernel.cpp");

  // 通过RTC（Runtime Compilation）编译kernel源码为device binary
  aclrtcProg prog;
  aclrtcCreateProg(&prog, source.c_str(), "kernel_name", 0, nullptr, nullptr);
  aclrtcCompileProg(prog, num_options, options);

  // 获取编译产物（device binary）
  size_t bin_size;
  aclrtcGetBinDataSize(prog, &bin_size);
  std::vector<uint8_t> binary(bin_size);
  aclrtcGetBinData(prog, binary.data());
 // 将编译产物缓存到成员变量，供后续Execute使用
  device_elves_[key] = std::move(binary);
  aclrtcDestroyProg(&prog);   // 销毁编译程序对象
  return GRAPH_SUCCESS;
}
```

- 上例为**示意代码**：RTC相关API的名称、签名随CANN版本演进（如aclrtcCreateProg/aclrtcCreateProgram），请以目标CANN版本头文件为准。
- 每个RTC调用都应检查返回值并释放失败时已创建的资源。
- kernel编译失败时可通过日志获取编译错误信息，详见[调试与常见问题](debug_and_common_issues.md)。

## PortableOp::Serialize/Deserialize

序列化/反序列化回调，用于OM模型下沉。

```c++
graphStatus Serialize(std::vector<uint8_t> &buffer) override;
graphStatus Deserialize(const std::vector<uint8_t> &buffer) override;
```

**典型流程（示意，buffer格式自定义）：**

```c++
// 序列化：将编译产物写入buffer，供ATC写入OM模型文件
// buffer格式完全由用户自定义，GE只透传不解析
graphStatus Serialize(std::vector<uint8_t> &buffer) override {
  // 序列化格式：[count][key_len][key][bin_len][bin]...
  size_t offset = 0;
  buffer.resize(CalcTotalSize());

  // 写入binary数量
  uint32_t count = device_elves_.size();
  memcpy(buffer.data() + offset, &count, sizeof(count));
  offset += sizeof(count);

  // 逐个写入每个binary的key和数据
  for (auto &kv : device_elves_) {
    // ... 写入key和binary
  }
  return GRAPH_SUCCESS;
}

// 反序列化：从OM模型文件的buffer中恢复编译产物
// 格式必须与Serialize严格对称
graphStatus Deserialize(const std::vector<uint8_t> &buffer) override {
  size_t offset = 0;

  // 读取binary数量
  uint32_t count;
  memcpy(&count, buffer.data() + offset, sizeof(count));
  offset += sizeof(count);

  // 逐个读取每个binary的key和数据
  for (uint32_t i = 0; i < count; ++i) {
    // ... 读取key和binary，存入device_elves_
  }
  return GRAPH_SUCCESS;
}
```

**关键点：**

- buffer格式完全由用户自定义，GE不解析只透传。
- Serialize在ATC编译阶段被调用，将Compile产物写入buffer。
- Deserialize在OM加载执行阶段被调用，从buffer恢复Compile产物。
- 多binary场景建议序列化格式：\[count\]\[key\_len\]\[key\]\[bin\_len\]\[bin\]...。

## ArgsUpdater::UpdateHostArgs

I/O地址变化时的回调，用于刷新kernel args中的地址引用。

```c++
graphStatus UpdateHostArgs(gert::UpdateArgsContext *ctx) override;
```

**上下文API：**

| 方法 | 用途 |
| --- | --- |
| ctx->GetKernelArgs(placement, index) | 获取指定位置的kernel args。 |
| 继承自EagerOpExecutionContext的所有方法 | 获取输入/输出Tensor等。 |

**适用场景：**  静态图下沉后，模型加载时I/O地址可能与编译时不同，需要刷新args buffer中的地址。
