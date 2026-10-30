# FuseCheckResult

<!-- md-trans-meta sourceCommit=6d5a35469af253d7c5b6ff894beab0546bf42c74 translatedAt=2026-10-15T03:47:21.346Z pushedAt=2026-10-16T10:53:07.584Z -->

## Applicable Products

Supported on all chips.

## Description

Describes the fusion feasibility check result of a node set in the form of a frozen dataclass.

## Prototype

```python
@dataclass(frozen=True)
class FuseCheckResult:
    ok: bool
    reason: str = ""
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| ok | Output | **bool** type. Whether the node set can be fused. |
| reason | Output | String type. Reason why the node set cannot be fused; an empty string when `ok=True`. |

## Constraints

The object is immutable, and its attributes cannot be modified after creation.
