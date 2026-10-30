# SetGraphFixedFeatureMemoryBaseWithType

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-26T13:25:12.369Z pushedAt=2026-10-28T07:42:19.831Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Specifies the Fixed Feature memory base address for different memory types of a graph.

The memory size is obtained from the [GetCompiledGraphSummary](GetCompiledGraphSummary.md)**\>GetAllFeatureMemoryTypeSize** API. Feature memory refers to the intermediate memory required during model execution (such as the input and output memory of intermediate nodes).

- When a graph contains Fixed memory but you do not specify it, Fixed memory is allocated by default.
- You can disable the default Fixed memory application by setting the address to nullptr and the size to 0 (this scenario is not supported for the memory type MEMORY\_TYPE\_P2P).

## Prototype

```c++
Status SetGraphFixedFeatureMemoryBaseWithType(uint32_t graph_id, MemoryType type, const void *const memory, size_t size)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |
| type | Input | Memory type, including [MemoryType](../MemoryType.md)::MEMORY_TYPE_DEFAULT and **MemoryType::MEMORY_TYPE_P2P**. |
| memory | Input | Fixed Feature memory base address to set. |
| size | Input | Fixed Feature memory size to set. It must be greater than or equal to the size obtained from the GetCompiledGraphSummary>GetAllFeatureMemoryTypeSize API. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

- Before calling this API, you must call the [CompileGraph](CompileGraph.md) API to compile the graph.
- Each memory type of each graph can be set only once and cannot be refreshed.
- The Fixed memory address whose memory type is set to **MEMORY\_TYPE\_DEFAULT** cannot be used together with the [UpdateGraphFeatureMemoryBase](UpdateGraphFeatureMemoryBase.md) API.
- This API cannot be used together with the [SetGraphFixedFeatureMemoryBase](SetGraphFixedFeatureMemoryBase.md) API. SetGraphFixedFeatureMemoryBase sets the base address and size of Fixed memory, whereas SetGraphFixedFeatureMemoryBaseWithType sets the base address of Fixed memory for **different memory types**.
- When the Fixed memory length is 0, calling this API does not take effect, and a warning log is generated to notify you.
- When the **ge.exec.staticMemoryPolicy** parameter in [options parameter description](../options_params/memory_management.md) > memory management is set to **4**, or the [GE\_USE\_STATIC\_MEMORY](../../../../../user_guides/env_vars/GE_USE_STATIC_MEMORY.md) environment variable is set to 4, dynamic and static graphs support memory reuse. However, this API does not support address refresh. Therefore, in the dynamic and static graph memory reuse scenario, you cannot disable the default Fixed memory allocation by setting the address to nullptr and the size to 0.
