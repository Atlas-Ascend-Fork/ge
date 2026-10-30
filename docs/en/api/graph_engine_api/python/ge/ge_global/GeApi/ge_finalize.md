# ge\_finalize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:58:45.925Z pushedAt=2026-10-16T10:53:07.409Z -->

## Description

Finalizes GE and releases all resources.

## Prototype

```python
ge_finalize() -> int
```

## Parameters

None

## Return Value

\(int\) Returns the finalization result, where 0 indicates success.

## Constraints

If finalization fails, **RuntimeError** is thrown.
