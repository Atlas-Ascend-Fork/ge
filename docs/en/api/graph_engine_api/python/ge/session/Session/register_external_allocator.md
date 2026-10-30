# register\_external\_allocator

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-15T04:17:20.186Z pushedAt=2026-10-16T10:53:07.719Z -->

## Description

The user registers their own **Allocator** with **GE**, which is applicable to scenarios where the user's memory pool is used.

## Prototype

```python
register_external_allocator(stream: int, allocator: Allocator) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **stream** | Input | Stream on which the Allocator is registered. |
| **allocator** | Input | User Allocator object, derived from [Allocator](../../allocator/Allocator/Allocator.md). |

## Return Value

None

## Constraints

- If **stream** is not an integer, **TypeError** is raised.
- If **allocator** is not an **Allocator** object, **TypeError** is raised.
- If registration fails, **RuntimeError** is raised.
- For the same stream, if this API is called multiple times, the last registration takes effect.
- For different streams, if the same **Allocator** is used, concurrent execution across multiple streams is not allowed. Before executing the next **Stream**, stream synchronization must be performed on the previous **Stream**.
- Before releasing the memory in the **Allocator** back to the operating system, call the “[acl.rt.synchronize\_stream](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/README.md)” API to perform stream synchronization to ensure that the tasks in the **Stream** have completed execution.
