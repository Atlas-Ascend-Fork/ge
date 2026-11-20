# GetContextValue

<!-- md-trans-meta sourceCommit=ebd84524f3722fcbb0fbf1dc47bb792f2e47e3e6 translatedAt=2026-11-17T02:44:06.436Z pushedAt=2026-11-18T02:41:08.882Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/op\_compile\_context.h\>
- Library file: liblowering.so

## Description

In the [`Compile`](../../ge/CompilableOp/Compile.md) callback, a custom operator can use this API to query key information that has been determined at compile time (such as the root graph ID, node ID, and available vector core quantity), and adjust its compile-time behavior accordingly. Parameters are uniformly interpreted by numeric semantics: a Boolean parameter has a value of 0 or 1, and a numeric type parameter has its corresponding numeric value. For parameters that cannot be determined at compile time (such as address refresh enablement, which is determined by the runtime memory type), the query returns false.

## Prototype

```c++
bool GetContextValue(ContextParam key, uint64_t &value) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| key | Input | Enumeration key of the interaction parameter. See the following table for its values. |
| value | Output | Parameter value obtained by the query. Not written out on query failure. |

| Value | Compile-time Semantics |
| --- | --- |
| kGraphId | Graph ID of the root graph to which the currently compiled submodel belongs. |
| kNodeId | Node ID of the current operator node within the graph, unique within the graph. |
| kVectorCoreNum | Quantity of vector cores available to the current operator node. When the operator attribute is not configured, it falls back to the global option and then to the platform default. The query fails when all sources are unavailable. |
| kAiCoreNum | Quantity of AI Cores available to the current operator node. When the operator attribute is not configured, it falls back to the global option and then to the platform default. The query fails when all sources are unavailable. |
| kAiCpuNum | Quantity of AICPU cores (the static platform `ai_cpu_cnt` is used at compile time). The query fails when it is not configured or fails to be obtained. |
| kStaticModel, kAddressRefreshEnabled | Not supported at compile time. The query returns false. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | bool | Returns true on a successful query. Returns false (without writing out value) when the key is undefined, the parameter is not configured, the value cannot be determined at compile time, or the current context is unavailable. |

## Constraints

- This API is valid only when called within the [`Compile`](../../ge/CompilableOp/Compile.md) callback. The parameters reflect the graph information of the current compilation, and the operator must not cache them across compilation tasks. Unset parameters always return false. A numeric value of 0 is a valid value (for example, a graph ID of 0), so the return value should be used to distinguish "value is 0" from "not configured".
