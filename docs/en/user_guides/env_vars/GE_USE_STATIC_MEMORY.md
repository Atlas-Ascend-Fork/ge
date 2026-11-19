# GE_USE_STATIC_MEMORY

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-11-13T10:00:23.634Z pushedAt=2026-11-16T06:11:08.647Z nl=2 -->

## Feature Description

**This environment variable will be deprecated in later versions.**

Specifies the memory allocation mode used during network running. The following values are supported:

- 0 (default): Dynamically allocates memory based on the actual size.
- 2: Supports dynamic memory expansion for static shapes. During network running, this value enables memory reuse across multiple graphs in the same session, that is, memory is allocated based on the largest graph. For example, if the memory required by the graph currently being executed exceeds that of the previous graph, the memory of the previous graph is directly released and reallocated based on the memory required by the current graph.
- 3: Supports dynamic memory expansion for dynamic shapes, which resolves the fragmentation issue during dynamic memory allocation and reduces the memory usage of dynamic-shape networks.
- 4: Supports dynamic memory expansion for both static shapes and dynamic shapes.

> [!NOTE]Note
>
> - When multiple images are executed concurrently, configuration as **2** and **4** is not supported.
> - For compatibility with historical version configurations, when configured as **1**, the system processes it according to the **2** method.
> - When configured as **3** and **4**, buffer benefits are achieved, but performance loss may occur.
> - For TensorFlow training scenarios and online inference scenarios, this environment variable and the parameter **static_memory_policy** cannot be used simultaneously; otherwise, the network runtime will conflict. You are advised to use **static_memory_policy** to set the buffer allocation method of the network runtime.
> - For graph development scenarios, this environment variable and the configuration parameter [ge.exec.staticMemoryPolicy](../../api/graph_engine_api/cpp/ge/options_params/memory_management.md#geexecstaticmemorypolicy) cannot be used simultaneously; otherwise, the network runtime will conflict. You are advised to use **ge.exec.staticMemoryPolicy** to set the buffer allocation method of the network runtime.

## Configuration Example

```bash
export GE_USE_STATIC_MEMORY=2
```

## Constraints

None

## Applicable Products

All chips supported
