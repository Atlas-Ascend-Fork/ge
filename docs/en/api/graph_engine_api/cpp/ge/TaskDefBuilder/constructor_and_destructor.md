# Constructors and Destructors

<!-- md-trans-meta sourceCommit=c38756906bc8995090581c397cd60a0f922bfd78 translatedAt=2026-11-17T03:03:38.555Z pushedAt=2026-11-18T02:41:08.933Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header File: \#include <exe_graph/runtime/task\_def\_builder.h\>
- Library file: libregister.so

## Description

The constructor creates a TaskDefBuilder object and internally creates an empty task description (held by Pimpl). The destructor releases the internally held task description object.

## Prototype

```c++
TaskDefBuilder()
~TaskDefBuilder()
TaskDefBuilder(const TaskDefBuilder &) = delete
TaskDefBuilder &operator=(const TaskDefBuilder &) = delete
```

## Parameters

None

## Return Value

None

## Constraints

- TaskDefBuilder is non-copyable and non-assignable.
- The internal task description object is released uniformly upon destruction. The view object does not participate in the release.

## Examples

```c++
// Construct the task in the GenTask callback.
gert::TaskDefBuilder task;
task.SetStreamId(context->GetStreamId()).SetType(RT_MODEL_TASK_KERNEL);
...
```
