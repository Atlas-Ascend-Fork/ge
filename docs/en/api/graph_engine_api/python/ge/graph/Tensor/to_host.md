# to\_host

<!-- md-trans-meta sourceCommit=5f639ce5c040cc9acb4c51426dd643c1cfc70c83 translatedAt=2026-11-17T02:39:40.255Z pushedAt=2026-11-18T02:41:08.860Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Moves the current **Tensor** from the **Device** to the **Host**.

## Prototype

```python
to_host() -> Tensor
```

## Parameters

None

## Return Value

\(Tensor\) Returns itself to support chained calls.

## Constraints

- If the current **Tensor** is not in **Device** memory, a **ValueError** is raised.
- If the move fails, a **RuntimeError** is raised.
