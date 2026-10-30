# unregister\_external\_allocator

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:19:49.444Z pushedAt=2026-10-16T10:53:07.729Z -->

## Description

Destroys the **Allocator** registered by the user based on **Stream**. This applies to scenarios where the user's memory pool is used.

## Prototype

```python
unregister_external_allocator(stream: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **stream** | Input | Graph ID. |

## Return Value

None

## Constraints

- If **stream** is not an integer, a **TypeError** is thrown.
- If destruction fails, a **RuntimeError** is thrown.
- For more constraints, see [Constraints](../../../../cpp/ge/Session/UnregisterExternalAllocator.md#constraints).
