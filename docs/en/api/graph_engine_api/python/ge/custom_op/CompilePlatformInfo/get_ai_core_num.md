# get\_ai\_core\_num

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:32:06.755Z pushedAt=2026-10-16T10:53:07.307Z -->

## Applicable Products

Supported on all chips.

## Description

Queries the number of AI Cores on the current compilation platform.

## Prototype

```python
get_ai_core_num() -> int
```

## Parameters

None

## Constraints

This function can only be called within the current synchronous `compile` callback.

## Examples

```python
core_num = platform.get_ai_core_num()
```
