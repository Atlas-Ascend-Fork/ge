# get\_core\_num

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:33:03.744Z pushedAt=2026-10-16T10:53:07.310Z -->

## Applicable Products

Supported on all chips.

## Description

Queries the number of cores on the platform.

## Prototype

```python
get_core_num(core_type: str | None = None) -> int
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| core_type | Input | Optional non-empty core type name. When it is `None`, the default number of cores of the platform is queried. |

## Constraints

Can only be called within the current synchronous `compile` callback.

## Examples

```python
core_num = platform.get_core_num()
```
