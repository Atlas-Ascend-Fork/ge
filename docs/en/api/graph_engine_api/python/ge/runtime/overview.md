# Overview

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:09:30.653Z pushedAt=2026-10-16T10:53:07.687Z -->

`ge.runtime` provides runtime data structures used for Python custom operator execution and metadata inference. `TensorDesc` is used for the input and return values of the `register_op` decorated function; objects such as `Tensor`, `StorageShape`, and `StorageFormat` are provided by the runtime context during callbacks.

`ge.runtime.TensorDesc` is a different type from [`ge.graph.TensorDesc`](../graph/TensorDesc/overview.md) in the graph construction API, and their usage scenarios and interfaces must not be mixed.

- [`Tensor`](Tensor/overview.md): Tensor runtime view used during callback execution.
- [`TensorDesc`](TensorDesc/overview.md): Tensor metadata used by the `infer_meta` of a Python custom operator.
