# UnregisterExternalAllocator

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:26:55.035Z pushedAt=2026-10-28T07:42:19.836Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Destroys the **Allocator** registered by the user based on **Stream**. This applies to scenarios where the user's memory pool is used.

## Prototype

```c++
Status UnregisterExternalAllocator(const void *const stream) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| stream | Input | Stream on which the user Allocator to be destroyed resides. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: The setting is successful.<br>**FAILED**: The setting fails. |

## Constraints

- Before destroying the Allocator, call this API to unregister it.
- If the Stream to be unregistered does not exist, or this API is called multiple times for unregistration, this API performs no operation and returns success.
