# UnregisterExternalAllocator

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:56:33.040Z pushedAt=2026-10-28T07:40:22.668Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Destroys the **Allocator** registered by the user based on **Stream**. This applies to scenarios where the user's memory pool is used.

## Prototype

```c++
Status UnregisterExternalAllocator(const void *const stream) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| stream | Input | Specifies the stream on which the user **Allocator** to be destroyed is located. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Setting succeeded.<br>**FAILED**: Setting failed. |

## Constraints

- Before the user destroys the **Allocator**, call this API to unregister it.
- If the **Stream** to be unregistered does not exist, or if this API is called multiple times to unregister, this API performs no operation internally and returns success.
