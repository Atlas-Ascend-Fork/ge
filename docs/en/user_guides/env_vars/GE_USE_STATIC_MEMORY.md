# GE_USE_STATIC_MEMORY

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-20T12:51:31.135Z pushedAt=2026-10-28T11:33:48.738Z nl=2 -->

## Feature Description

**This environment variable will be deprecated in later versions.**

Specifies the memory allocation mode used during network running. The following values are supported:

- 0 (default): Dynamically allocates memory based on the actual size.
- 2: Supports dynamic memory expansion for static shapes. During network running, this value enables memory reuse across multiple graphs in the same session, that is, memory is allocated based on the largest graph. For example, if the memory required by the graph currently being executed exceeds that of the previous graph, the memory of the previous graph is directly released and reallocated based on the memory required by the current graph.
- 3: Supports dynamic memory expansion for dynamic shapes, which resolves the fragmentation issue during dynamic memory allocation and reduces the memory usage of dynamic-shape networks.
- 4: Supports dynamic memory expansion for both static shapes and dynamic shapes.

> [!NOTE]
>
> - When multiple graphs are executed concurrently, values "2" and "4" are not supported.
> - For compatibility with historical version configurations, when the value is set to "1", the system processes it in the same way as "2".
> - When the value is set to "3" or "4", memory benefits are achieved, but performance loss may occur.
> - For TensorFlow training and online inference scenarios, this environment variable cannot be used together with the configuration parameter "static_memory_policy"; otherwise, network running will conflict. You are advised to use the configuration parameter **static_memory_policy** to set the memory allocation mode during network running.
> - For graph development scenarios, this environment variable cannot be used together with the configuration parameter [ge.exec.staticMemoryPolicy](../../api/graph_engine_api/cpp/ge/options_params/memory_management.md#geexecstaticmemorypolicy); otherwise, network running will conflict. It is recommended that developers preferentially use the configuration parameter "ge.exec.staticMemoryPolicy" to set the memory allocation mode during network running.

## Configuration Example

```bash
export GE_USE_STATIC_MEMORY=2
```

## Usage Constraints

None

## Applicable Products

Supported on all chips
