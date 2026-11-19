# SerializeToArray

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:06:55.430Z pushedAt=2026-11-18T02:41:08.940Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

Serializes the task description into a byte stream and writes it to the user buffer. The serialization result is byte-for-byte identical to that of domi::TaskDef::SerializeToArray, and the consumer can deserialize it directly via ParseFromArray.

## Prototype

```c++
bool SerializeToArray(uint8_t *data, size_t size) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| data | Output | Serialization output buffer. It is recommended to allocate it via [ByteSizeLong](ByteSizeLong.md). |
| size | Input | Buffer length (bytes). |

## Return Value

- true: Serialization succeeded.
- false: Serialization failed (internal object creation failed, data is empty, or size is 0). The caller should return a task generation failure.

## Constraints

The buffer length should not be less than the return value of [ByteSizeLong](ByteSizeLong.md).

## Examples

```c++
// In the GenTask callback: context is the callback input parameter (gert::ExeResGenerationContext*), and tasks is the callback output parameter.
// RT_MODEL_TASK_KERNEL is the operator-side local constant 0 (Kernel-type task).
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
