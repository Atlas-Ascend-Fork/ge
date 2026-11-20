# get\_platform\_resource\_group

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:33:16.525Z pushedAt=2026-10-16T10:53:07.312Z -->

## Applicable Products

Supported on all chips.

## Description

Queries a platform resource group.

## Prototype

```python
get_platform_resource_group(group: str) -> dict[str, str]
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **group** | Input | Non-empty platform resource group name. |

## Constraints

Can only be called within the current synchronous `compile` callback.

## Examples

```python
resources = platform.get_platform_resource_group("ai_core")
```
