# GetContextValue

<!-- md-trans-meta sourceCommit=ebd84524f3722fcbb0fbf1dc47bb792f2e47e3e6 translatedAt=2026-11-17T02:42:32.110Z pushedAt=2026-11-18T02:41:08.876Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

In the [`Execute`](../../ge/EagerExecuteOp/Execute.md) callback, a custom operator can query key information about the current execution context via this API, such as whether it is running on a static graph, whether the address needs refresh, and the root graph id to which it belongs, and adaptively adjust its execution behavior accordingly. Parameters are uniformly interpreted by numeric semantics: Boolean parameters take the value 0 or 1, and numeric parameters take the corresponding numeric value.

## Prototype

```c++
bool GetContextValue(ContextParam key, uint64_t &value) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| key | Input | Enumeration key of the interaction parameter. See the following table for its value. |
| value | Output | Queried parameter value. Not written out on query failure. |

**ContextParam value description:**

| Value | Semantics |
| --- | --- |
| kStaticModel | Whether running on static graph (including static subgraphs with dynamic shapes), with a value of 0 or 1. |
| kAddressRefreshEnabled | Whether the output address executed in this round needs refresh, with a value of 0 or 1. |
| kGraphId | Graph id of the root graph to which the current model belongs. Always the root graph value, unaffected by subgraphs. |
| kNodeId | Node id of the current operator node within the graph, unique within the graph. |
| kVectorCoreNum | Number of vector cores available to the current operator node. When the operator attribute is not configured, it falls back in the order of global option > platform default; query fails when all sources are unavailable. |
| kAiCoreNum | Number of AI Cores available to the current operator node. When the operator attribute is not configured, it falls back in the order of global option > platform default; query fails when all sources are unavailable. |
| kAiCpuNum | Number of AICPU cores available to the current operator node (device value). Query fails when not configured or when acquisition fails. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | bool | Returns true on successful query; returns false when the key is undefined, the parameter is not configured, or the current context is unavailable (value is not written out). |

## Constraints

- Valid only when called within the [`Execute`](../../ge/EagerExecuteOp/Execute.md) callback. The parameter value reflects a snapshot of the context executed in this round, and the operator should not cache it for use across rounds. A numeric value of 0 is a valid value (for example, a graph id of 0), so the return value should be used to distinguish "value is 0" from "not configured" rather than only checking whether value is 0.
