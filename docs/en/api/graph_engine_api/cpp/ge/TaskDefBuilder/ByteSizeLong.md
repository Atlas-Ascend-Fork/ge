# ByteSizeLong

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:03:03.710Z pushedAt=2026-11-18T02:41:08.932Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

Obtains the byte length of the current task description after serialization, which is used to allocate the serialization buffer. The result is consistent with that of domi::TaskDef::ByteSizeLong.

## Prototype

```c++
size_t ByteSizeLong() const
```

## Parameters

None

## Return Value

Returns the serialized byte length; returns 0 if the internal object fails to be created.

## Constraints

None

## Examples

```c++
// Construct the task in the GenTask callback.
gert::TaskDefBuilder task;
...
std::vector<uint8_t> buf(task.ByteSizeLong(), 0);
task.SerializeToArray(buf.data(), buf.size());
```
