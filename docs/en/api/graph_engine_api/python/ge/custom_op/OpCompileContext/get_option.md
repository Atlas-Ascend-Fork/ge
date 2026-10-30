# get\_option

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:38:15.928Z pushedAt=2026-10-16T10:53:07.333Z -->

## Applicable Products

Supported on all chips.

## Description

Queries the **option** in the current graph compilation context.

## Prototype

```python
get_option(option_key: str) -> str
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| option_key | Input | Non-empty **option** name. |

## Constraints

Can only be called within the current synchronous `compile` callback.

## Examples

```python
option = ctx.get_option("custom.compile.option")
```
