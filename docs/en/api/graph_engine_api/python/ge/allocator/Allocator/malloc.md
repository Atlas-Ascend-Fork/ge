# malloc

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:20:28.209Z pushedAt=2026-10-16T10:53:07.261Z -->

## Applicable Products

Supported on all chips.

## Description

Allocates device memory of the specified **size** from the user memory pool.

## Prototype

```python
malloc(size: int) -> MemBlock
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| size | Input | Specifies the size of memory to be allocated. |

## Return Value

\([MemBlock](../MemBlock/MemBlock.md)\) Returns a memory **Block** object.

## Constraints

The abstract method must be implemented by the user.
