# to\_device

<!-- md-trans-meta sourceCommit=5f639ce5c040cc9acb4c51426dd643c1cfc70c83 translatedAt=2026-11-17T02:39:36.494Z pushedAt=2026-11-18T02:41:08.859Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Moves the current **Tensor** from Host to Device.

## Prototype

```python
to_device() -> Tensor
```

## Parameters

None

## Return Value

\(Tensor\) Returns itself to support chained calls.

## Constraints

- If the current **Tensor** is not in Host memory, a **ValueError** is raised.
- If the move fails, a **RuntimeError** is raised.
