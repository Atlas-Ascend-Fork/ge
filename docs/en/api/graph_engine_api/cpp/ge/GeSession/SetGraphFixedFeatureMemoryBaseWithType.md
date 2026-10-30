# SetGraphFixedFeatureMemoryBaseWithType

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-16T13:55:24.827Z pushedAt=2026-10-28T07:40:22.670Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api\_v2.h\>
- Library file: libge\_runner\_v2.so

## Description

Specifies the Fixed Feature memory base address for different memory types of a graph.

Before calling this API, you must first call [CompileGraph](./CompileGraph.md) to compile the graph. The memory size is obtained from the [GetCompiledGraphSummary](./GetCompiledGraphSummary.md)**\>GetAllFeatureMemoryTypeSize** API. Feature memory refers to the intermediate memory required during model execution (for example, the input and output memory of intermediate nodes).

- When the graph contains Fixed memory but the user does not specify it, Fixed memory is allocated by default.
- The user can disable the default Fixed memory allocation by setting the address to nullptr and the size to 0 (this scenario is not supported when the memory type is **MEMORY\_TYPE\_P2P**).

## Prototype

```c++
Status SetGraphFixedFeatureMemoryBaseWithType(uint32_t graph_id, MemoryType type, const void *const memory, size_t size)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |
| type | Input | Memory type, including [MemoryType](../MemoryType.md)::MEMORY_TYPE_DEFAULT and MemoryType::MEMORY_TYPE_P2P. |
| memory | Input | Fixed Feature memory base address to be set. |
| size | Input | Size of the Fixed Feature memory to be set. It must be greater than or equal to the size obtained from the GetCompiledGraphSummary>GetAllFeatureMemoryTypeSize API. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | SUCCESS: Set successfully.<br>FAILED: Failed to set. |

## Constraints

- Each memory type of each graph can be set only once, and refreshing is not supported.
- Setting the Fixed memory address with the memory type **MEMORY\_TYPE\_DEFAULT** cannot be used together with the [UpdateGraphFeatureMemoryBase](./UpdateGraphFeatureMemoryBase.md) API.
- When the Fixed memory length is 0, calling this API does not take effect, and a warning log is generated to notify the user.
- When the **ge.exec.staticMemoryPolicy** parameter in [options parameter description](../options_params/memory_management.md) \> memory management is set to 4, or the [GE\_USE\_STATIC\_MEMORY](../../../../../user_guides/env_vars/GE_USE_STATIC_MEMORY.md) environment variable is set to 4, dynamic and static graphs support memory reuse. However, this API does not support address refresh. Therefore, in the dynamic and static graph memory reuse scenario, disabling the default Fixed memory allocation behavior by setting the address to **nullptr** and the size to 0 is not supported.
