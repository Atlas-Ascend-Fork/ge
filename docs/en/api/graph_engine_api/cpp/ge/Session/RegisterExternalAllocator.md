# RegisterExternalAllocator

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-10-26T13:20:36.601Z pushedAt=2026-10-28T07:42:19.816Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

The user registers their own **Allocator** with **GE**, which is applicable to scenarios where the user's memory pool is used.

This API must be used together with [LoadGraph](LoadGraph.md), [RunGraphWithStreamAsync](RunGraphWithStreamAsync.md), or [ExecuteGraphWithStreamAsync](ExecuteGraphWithStreamAsync.md), and it must be registered before LoadGraph is called.

## Prototype

```c++
Status RegisterExternalAllocator(const void *const stream, AllocatorPtr allocator) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| stream | Input | Stream on which the Allocator is registered. |
| allocator | Input | Smart pointer to the user-defined Allocator object. The Allocator is derived from [Allocator](https://gitcode.com/cann/metadef/blob/9.2.0/docs/en/api/README.md). |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: The setting is successful.<br>**FAILED**: The setting fails. |

## Constraints

- For the same stream, if this API is called multiple times, the last registration takes effect.
- For different streams, if the same **Allocator** is used, concurrent execution across multiple streams is not allowed. Before executing the next **Stream**, stream synchronization must be performed on the previous **Stream**.
- Before releasing the memory in the Allocator to the operating system, call "aclrtSynchronizeStream" to synchronize the stream and ensure that all tasks on the stream have completed.

    For details about the API, see "Stream Management" in *Runtime API*.
