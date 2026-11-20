# ge\_finalize

<!-- md-trans-meta sourceCommit=5f639ce5c040cc9acb4c51426dd643c1cfc70c83 translatedAt=2026-11-17T02:39:44.116Z pushedAt=2026-11-18T02:41:08.862Z nl=2 -->

## Description

Finalizes GE and releases all resources.

## Prototype

```python
ge_finalize() -> int
```

## Parameters

None

## Return Value

\(int\) Returns the finalization result, where 0 indicates success.

## Constraints

If finalization fails, **RuntimeError** is thrown.
