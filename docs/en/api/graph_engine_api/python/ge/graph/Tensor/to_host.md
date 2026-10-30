# to\_host

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:24:38.044Z pushedAt=2026-10-16T10:53:07.515Z -->

## Applicable Products

Supported on all chips.

## Description

Moves the current **Tensor** from the **Device** to the **Host**.

## Prototype

```python
to_host() -> Tensor
```

## Parameters

None

## Return Value

\(Tensor\) Returns itself to support chained calls.

## Constraints

- If the current **Tensor** is not in **Device** memory, a **ValueError** is raised.
- If the move fails, a **RuntimeError** is raised.
