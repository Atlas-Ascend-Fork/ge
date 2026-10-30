# Callback Interface

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:33:08.160Z pushedAt=2026-10-28T03:30:34.492Z nl=2 -->

## API Overview

| API | Method | Trigger Time | Required |
| --- | --- | --- | --- |
| EagerExecuteOp | Execute | Executes the kernel at runtime | Required in all scenarios |
| ShapeInferOp | InferShape/InferDataType | Shape/DataType inference at compile time | Optional |
| CompilableOp | Compile | RTC compilation of the kernel at compile time | Scenario 2/Scenario 3 |
| PortableOp | Serialize / Deserialize | ATC writes OM/OM loading and restoration | Scenario 3 |
| ArgsUpdater | UpdateHostArgs | Refreshes when I/O addresses change | Static graph offloading |

## EagerExecuteOp::Execute

Runtime execution callback, the most core API, **must be implemented in all scenarios**.

```c++
graphStatus Execute(gert::EagerOpExecutionContext *ctx) override;
```

**Context APIs:**

| API | Description |
| --- | --- |
| ctx->GetInputTensor(index) | Obtain the input Tensor (including addr, shape, dtype, and format) |
| ctx->MallocOutputTensor(index, shape, format, dtype) | Allocate device memory for the output Tensor |
| ctx->MallocWorkSpace(size) | Allocate device memory for the workspace |
| ctx->GetStream() | Obtain the execution stream |
| ctx->GetOutputTensor(index) | Obtain the allocated output Tensor |
| ctx->MakeOutputRefInput(out_idx, in_idx) | Make the output reference the input address (zero-copy) |

**Typical flow:**

```c++
graphStatus Execute(gert::EagerOpExecutionContext *ctx) {
  // Obtain the 0th input Tensor (const, not modifiable)
  auto *x = ctx->GetInputTensor(0);
  // Allocate memory for the output Tensor, with shape/format/dtype inherited from the input
  auto *y = ctx->MallocOutputTensor(0, x->GetShape(), x->GetFormat(), x->GetDataType());
  // Obtain the execution stream, on which the kernel runs asynchronously
  void *stream = ctx->GetStream();
  // Launch the user-defined kernel
  LaunchMyKernel(x->GetAddr(), y->GetAddr(), stream);
  return GRAPH_SUCCESS;
}
```

**Notes:**

- The output memory is managed by the context, and the caller does not need to release it.
- GetInputTensor returns const Tensor\*, and the input data cannot be modified.
- For the precompiled kernel binary scenario, load the binary in Execute and launch it through aclrtLaunchKernelWithHostArgs.
- **Note that the launch parameter refers to the number of elements rather than the number of bytes**: the number of elements uses GetShapeSize\(\), while GetSize\(\) returns the size in bytes (for details, see [Quick Start](quick_start.md)).

## ShapeInferOp::InferShape/InferDataType

Callbacks for Shape and DataType inference at compile time.

```c++
graphStatus InferShape(gert::InferShapeContext *ctx) override;
graphStatus InferDataType(gert::InferDataTypeContext *ctx) override;
```

**InferShapeContext API:**

| Method | Purpose |
| --- | --- |
| ctx->GetInputShape(index) | Obtain the input origin Shape (const Shape*) |
| ctx->GetOutputShape(index) | Obtain the modifiable output Shape (Shape*) |
| ctx->GetInputTensor(index) | Obtain the input Tensor (data accessible) |

**InferDataTypeContext API:**

| Method | Purpose |
| --- | --- |
| ctx->GetInputDataType(index) | Obtain the input DataType |
| ctx->SetOutputDataType(index, dtype) | Set the output DataType |

**Typical flow:**

```c++
// Shape inference: set the output shape to be the same as the input shape.
graphStatus InferShape(gert::InferShapeContext *ctx) {
  auto *in_shape = ctx->GetInputShape(0);
  auto *out_shape = ctx->GetOutputShape(0);
  *out_shape = *in_shape;
  return GRAPH_SUCCESS;
}

// DataType inference: set the output type to be the same as the input type.
graphStatus InferDataType(gert::InferDataTypeContext *ctx) {
  return ctx->SetOutputDataType(0, ctx->GetInputDataType(0));
}
```

For a broadcast semantics example, see [Broadcast Semantics Example](concept_and_design.md#broadcast-semantics-example).

## CompilableOp::Compile

Online compilation callback, invoked during the GE/ATC compilation phase.

```c++
graphStatus Compile(gert::OpCompileContext *ctx) override;
```

**OpCompileContext API:**

| Method | Purpose |
| --- | --- |
| ctx->GetInputTensor(index) | Obtain the input tensor at compile time (including shape, dtype, and format) |
| ctx->GetOutputTensor(index) | Obtain the output tensor at compile time |
| ctx->GetOption(key, value) | Obtain compilation options |
| ctx->GetPlatformInfos(...) | Obtain target platform information (such as the chip model) |

**Typical flow:**

```c++
graphStatus Compile(gert::OpCompileContext *ctx) {
  // Obtain the input tensor at compile time (including metadata such as shape, dtype, and format)
  auto *input = ctx->GetInputTensor(0);
  // Generate a binary cache key based on the input shape to support kernel management in multi-shape scenarios
  auto key = BuildBinaryKey(input->GetShape());
 // Load the kernel source file
  auto source = LoadFile("my_kernel.cpp");

  // Compile the kernel source into a device binary through RTC (Runtime Compilation)
  aclrtcProg prog;
  aclrtcCreateProg(&prog, source.c_str(), "kernel_name", 0, nullptr, nullptr);
  aclrtcCompileProg(prog, num_options, options);

  // Obtain the compilation artifact (device binary)
  size_t bin_size;
  aclrtcGetBinDataSize(prog, &bin_size);
  std::vector<uint8_t> binary(bin_size);
  aclrtcGetBinData(prog, binary.data());
 // Cache the compilation artifact in a member variable for subsequent Execute calls
  device_elves_[key] = std::move(binary);
  aclrtcDestroyProg(&prog);   // Destroy the compilation program object
  return GRAPH_SUCCESS;
}
```

- The preceding example is **illustrative code**: the names and signatures of RTC-related APIs evolve across CANN versions (for example, aclrtcCreateProg/aclrtcCreateProgram). Refer to the header files of the target CANN version.
- Each RTC call should check the return value and release the resources already created upon failure.
- When kernel compilation fails, you can obtain the compilation error information from logs. For details, see [Debugging and Common Issues](debug_and_common_issues.md).

## PortableOp::Serialize/Deserialize

Serialization/deserialization callbacks, used for OM model offloading.

```c++
graphStatus Serialize(std::vector<uint8_t> &buffer) override;
graphStatus Deserialize(const std::vector<uint8_t> &buffer) override;
```

**Typical flow (illustrative; the buffer format is user-defined):**

```c++
// Serialization: write the compilation artifacts into the buffer for ATC to write into the OM model file
// The buffer format is fully user-defined; GE only passes it through without parsing
graphStatus Serialize(std::vector<uint8_t> &buffer) override {
  // Serialization format: [count][key_len][key][bin_len][bin]...
  size_t offset = 0;
  buffer.resize(CalcTotalSize());

  // Write the number of binaries
  uint32_t count = device_elves_.size();
  memcpy(buffer.data() + offset, &count, sizeof(count));
  offset += sizeof(count);

  // Write the key and data of each binary one by one
  for (auto &kv : device_elves_) {
    // ... Write the key and binary
  }
  return GRAPH_SUCCESS;
}

// Deserialization: restore the compilation artifacts from the buffer of the OM model file
// The format must be strictly symmetric with Serialize.
graphStatus Deserialize(const std::vector<uint8_t> &buffer) override {
  size_t offset = 0;

  // Read the number of binaries.
  uint32_t count;
  memcpy(&count, buffer.data() + offset, sizeof(count));
  offset += sizeof(count);

  // Read the key and data of each binary one by one.
  for (uint32_t i = 0; i < count; ++i) {
    // ... Read the key and binary, and store them into device_elves_.
  }
  return GRAPH_SUCCESS;
}
```

**Key points:**

- The buffer format is fully user-defined. GE only passes it through without parsing.
- Serialize is called during the ATC compile phase to write the Compile output into the buffer.
- Deserialize is called during the OM loading and execution phase to restore the Compile output from the buffer.
- For multi-binary scenarios, the recommended serialization format is: \[count\]\[key\_len\]\[key\]\[bin\_len\]\[bin\]....

## ArgsUpdater::UpdateHostArgs

Callback invoked when I/O addresses change, used to refresh the address references in kernel args.

```c++
graphStatus UpdateHostArgs(gert::UpdateArgsContext *ctx) override;
```

**Context APIs:**

| Method | Purpose |
| --- | --- |
| ctx->GetKernelArgs(placement, index) | Obtain the kernel args at the specified position. |
| All methods inherited from EagerOpExecutionContext | Obtain input/output tensors, etc. |

**Applicable Scenarioss:**  After a static graph is offloaded, the I/O addresses at model loading may differ from those at compile time, so the addresses in the args buffer need to be refreshed.
