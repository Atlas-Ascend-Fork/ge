# GetOption

<!-- md-trans-meta sourceCommit=ebd84524f3722fcbb0fbf1dc47bb792f2e47e3e6 translatedAt=2026-11-17T02:42:53.796Z pushedAt=2026-11-18T02:41:08.877Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/eager\_op\_execution\_context.h\>
- Library file: liblowering.so

## Description

Options are queried with three-level merge semantics across the global, session, and graph scopes (the graph scope has the highest priority), allowing a custom operator to read configuration items that affect its own behavior (such as `ge.exec.atomicCleanPolicy` and `ge.hardwareInfo`) in the [`Execute`](../../ge/EagerExecuteOp/Execute.md) callback without needing to know the source scope of the option.

## Prototype

```c++
ge::graphStatus GetOption(const ge::AscendString &option_key, ge::AscendString &option_value) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| option_key | Input | Option key, such as `ge.exec.atomicCleanPolicy`. |
| option_value | Output | The queried option value. It is not written out when the query fails. |

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | ge::graphStatus | Returns `GRAPH_SUCCESS` on a successful query; returns a failure status code when the option is not configured or the current context is unavailable (the value is not written out). |

## Constraints

- This API is valid only when called within the [`Execute`](../../ge/EagerExecuteOp/Execute.md) callback. It reads the options visible in the current execution context. Operators should not rely on cross-round consistency. Option values carry string semantics, and their numeric meanings are defined by the options themselves, so operators must parse them on their own.
- The V2 executor passes only the following options through to custom operators:
  - `ge.exec.staticMemoryPolicy`
  - `ge.exec.atomicCleanPolicy`
  - `ge.hardwareInfo`
- Querying any other key on the asynchronous execution thread returns a failure.
