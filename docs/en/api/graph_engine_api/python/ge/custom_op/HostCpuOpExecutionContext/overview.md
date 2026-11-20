# Overview

<!-- md-trans-meta sourceCommit=3bb5c44a2b84bf843658519f3b19948450f79744 translatedAt=2026-10-15T02:37:36.547Z pushedAt=2026-10-16T10:53:07.330Z -->

`HostCpuOpExecutionContext` is the runtime execution context used by the host `execute` of a Python custom operator. Users obtain this object through [get_execute_ctx](../get_execute_ctx.md) and cannot directly construct it.

This object is a borrowed view that is valid only within the current `execute`. Through this object, you can query output Tensors, allocate outputs, and have outputs reference inputs. After `execute` completes, the context and the borrowed objects it returns become invalid.
