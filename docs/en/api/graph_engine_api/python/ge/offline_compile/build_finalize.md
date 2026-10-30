# build\_finalize

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:30:09.225Z pushedAt=2026-10-16T10:53:07.540Z -->

## Applicable Products

Supported on all chips.

## Description

After the system completes model construction, this interface releases resources.

## Prototype

```python
build_finalize() -> None
```

## Parameters

None

## Return Value

None

## Constraints

This interface is called after [build\_initialize](build_initialize.md) and can be called only once when the process exits.
