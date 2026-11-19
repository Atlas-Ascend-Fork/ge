# ContextParam

<!-- md-trans-meta sourceCommit=ebd84524f3722fcbb0fbf1dc47bb792f2e47e3e6 translatedAt=2026-11-17T02:41:48.364Z pushedAt=2026-11-18T02:41:08.873Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Enumeration of context interaction parameters between GE and custom operators, used as the query key of the [`GetContextValue`](GetContextValue.md) API. The runtime and compilation contexts are shared. Parameters are uniformly valued by numeric semantics: Boolean parameters take the value 0 or 1, and numeric parameters take the corresponding value.

## Enum Definition

```c++
enum class ContextParam : uint32_t
```

## Enum Value Description

| Enum Value | Semantics |
| --- | --- |
| kStaticModel | Whether the operator runs in a static graph (including static subgraphs with dynamic shapes). The value is 0 or 1. |
| kAddressRefreshEnabled | Whether the output address of the current execution needs to be refreshed. The value is 0 or 1. |
| kGraphId | The graph id of the root graph to which the current model belongs. It always takes the value of the root graph and is not affected by subgraphs. |
| kNodeId | The node id of the current operator node within the graph, which is unique within the graph. |
| kVectorCoreNum | The number of vector cores available to the current operator node. If the operator attribute is not configured, it falls back in the order of global option > platform default. If none of the sources is available, the query fails. |
| kAiCoreNum | The number of AI Cores available to the current operator node. If the operator attribute is not configured, it falls back in the order of global option > platform default. If none of the sources is available, the query fails. |
| kAiCpuNum | The number of AICPU cores available to the current operator node. If it is not configured or fails to be obtained, the query fails. |
| kNum | Reserved and not used as a query key. |

## Constraints

- The enumeration values are valid within the custom operator context callbacks (`Execute`/`Compile`). For the specific supported subset, see the corresponding `GetContextValue` documentation. Adding a new parameter only requires adding an enumeration value without changing the API signature.
