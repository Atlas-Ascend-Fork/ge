# get\_compile\_platform\_info

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:23:43.332Z pushedAt=2026-10-16T10:53:07.275Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the read-only view of platform information for the current schema-bound `compile` callback.

## Prototype

```python
get_compile_platform_info() -> CompilePlatformInfo
```

## Parameters

None

## Constraints

- Can only be called within the current synchronous `compile` callback; calling it outside the callback throws `RuntimeError`.
- The returned object becomes invalid after the callback returns or throws an exception.

## Examples

```python
from ge.custom_op import get_compile_platform_info


def compile(self, x, y) -> None:
    platform = get_compile_platform_info()
    soc_version = platform.get_soc_version()
```
