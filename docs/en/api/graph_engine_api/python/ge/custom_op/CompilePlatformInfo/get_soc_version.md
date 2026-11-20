# get\_soc\_version

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:33:55.818Z pushedAt=2026-10-16T10:53:07.314Z -->

## Applicable Products

Supported on all chips.

## Description

Queries the SoC version of the current compilation platform.

## Prototype

```python
get_soc_version() -> str
```

## Parameters

None

## Constraints

It can only be called within the current synchronous `compile` callback.

## Examples

```python
soc_version = platform.get_soc_version()
```
