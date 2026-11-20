# Overview

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:07:11.953Z pushedAt=2026-11-18T02:41:08.941Z nl=2 -->

gert::TaskDefBuilder is used to construct, in the GenTask callback, a task description that is byte-for-byte compatible with the domi::TaskDef wire format, and to serialize it into a byte stream for direct deserialization and consumption by the downstream task generation process. A Pimpl hierarchical view is adopted to encapsulate and isolate task.proto: TaskDefBuilder corresponds to the TaskDef message (root, with exclusive ownership), KernelDefBuilder corresponds to the KernelDef message (view), and KernelContextBuilder corresponds to the KernelContext message (view).

The hierarchical view maps one-to-one to the proto message tree, and the write path of a task field follows the message tree path. For example, `task.GetKernelBuilder().SetBlockDim(x).GetContextBuilder().SetArgsFormat(s)` corresponds to `TaskDef.kernel.context.args_format`.

## Header File to Include

```c++
#include <exe_graph/runtime/task_def_builder.h>
```

## TaskDefBuilder Public API

```c++
TaskDefBuilder()
~TaskDefBuilder()
TaskDefBuilder &SetStreamId(uint32_t stream_id)
TaskDefBuilder &SetType(uint32_t type)
KernelDefBuilder GetKernelBuilder() const
size_t ByteSizeLong() const
bool SerializeToArray(uint8_t *data, size_t size) const
```

## KernelDefBuilder Public API (View)

```c++
KernelDefBuilder &SetBlockDim(uint32_t block_dim)
KernelContextBuilder GetContextBuilder() const
```

## KernelContextBuilder Public API (View)

```c++
KernelContextBuilder &SetArgsFormat(const ge::AscendString &args_format)
```

## Constraints

- TaskDefBuilder is non-copyable and non-assignable (it holds exclusive ownership of the internal proto object, which is released upon destruction).
- View objects (KernelDefBuilder/KernelContextBuilder) do not own the child messages, and their lifetime must be covered by the parent TaskDefBuilder. Views can only be obtained through GetKernelBuilder()/GetContextBuilder() and cannot be constructed from scratch.
- Chained setters return references, and the return value can be ignored.
- This set of APIs is provided since CANN 9.3.0.
