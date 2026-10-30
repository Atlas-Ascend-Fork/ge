# Overview

<!-- md-trans-meta sourceCommit=0fa3324a9d01fb71b672f626d7d93f5735012fe9 translatedAt=2026-10-15T02:36:08.846Z pushedAt=2026-10-16T10:53:07.326Z -->

`EagerOpExecutionContext` is the runtime execution context used by the [execute](../execute.md) function in Python custom operator implementations. Users obtain this object through [get_execute_ctx](../get_execute_ctx.md) and cannot construct it directly.

This object is a borrowed view that is valid only within the current `execute`. Through this object, you can query the output `Tensor`, request output and workspace memory, and obtain the execution stream handle. After `execute` finishes, the context and the borrowed objects it returns become invalid.
