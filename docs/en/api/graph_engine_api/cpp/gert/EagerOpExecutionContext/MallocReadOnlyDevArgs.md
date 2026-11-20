# MallocReadOnlyDevArgs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T08:06:42.807Z pushedAt=2026-10-28T08:50:45.465Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Allocates device-side memory for the host-side args buffer and returns a read-only KernelArgs pointer.

Before model execution, this API synchronizes the host\_args content to the device side. After the API returns, the caller can immediately release host\_args without performing a manual Host-to-Device copy.

## Prototype

```c++
 const KernelArgs* MallocReadOnlyDevArgs(void *host_args, size_t args_size) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| host_args | Input | Host-side args buffer pointer. The caller can release it immediately after the API returns. |
| args_size | Input | Args buffer size (in bytes). |

## Return Value

Returns a read-only KernelArgs pointer, or nullptr on failure.

## Constraints

None
