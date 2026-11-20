# SetGraphFixedFeatureMemoryBase

<!-- md-trans-meta sourceCommit=0f9e5e70315a2093b02dc21846074a745fd9219a translatedAt=2026-10-26T13:25:25.182Z pushedAt=2026-10-28T07:42:19.833Z nl=2 -->

## Header File/Library File

- Header file: \#include <ge/ge\_api.h\>
- Library file: libge\_runner.so

## Description

Specifies the base address of the Fixed Feature memory of the graph.

The memory size is obtained from the [GetCompiledGraphSummary](GetCompiledGraphSummary.md) \> **GetFixedFeatureMemorySize** API.

Feature memory refers to the intermediate memory required during model execution (for example, the input and output memory of intermediate nodes).

- When the graph contains Fixed memory but you do not specify it, Fixed memory is allocated by default.
- You can disable the default Fixed memory allocation by setting the address to **nullptr** and size to **0**.

## Prototype

```c++
Status SetGraphFixedFeatureMemoryBase(uint32_t graph_id, const void * const memory, size_t size)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| graph_id | Input | ID of the graph. |
| memory | Input | Base address of the Fixed Feature memory to set. |
| size | Input | Size of the Fixed Feature memory to set. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Set successfully.<br>**FAILED**: Failed to set. |

## Constraints

- Before calling this API, you must call [CompileGraph](CompileGraph.md) to compile the graph.
- Each graph supports only one setting and does not support refreshing.
- This API cannot be used together with [UpdateGraphFeatureMemoryBase](UpdateGraphFeatureMemoryBase.md).
- This API cannot be used together with [SetGraphFixedFeatureMemoryBaseWithType](SetGraphFixedFeatureMemoryBaseWithType.md). **SetGraphFixedFeatureMemoryBaseWithType** is used to set the Fixed memory base address of **different memory types**, while **SetGraphFixedFeatureMemoryBase** is used to set the Fixed memory base address and size.
- When the Fixed memory length is 0, calling this API has no effect, and a warning log is generated to notify you.
- When the **ge.exec.staticMemoryPolicy** parameter in [options parameter description](../options_params/options_parameters_description.md) \> **Memory Management** is set to **4**, or the [GE\_USE\_STATIC\_MEMORY](../../../../../user_guides/env_vars/GE_USE_STATIC_MEMORY.md) environment variable is set to **4**, dynamic and static graphs support memory reuse. However, this API does not support address refreshing. Therefore, in the dynamic and static graph memory reuse scenario, you cannot disable the default Fixed memory allocation behavior by setting the address to **nullptr** and the **size** to **0**.
