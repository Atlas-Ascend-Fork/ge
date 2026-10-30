# get\_platform\_resource

<!-- md-trans-meta sourceCommit=1e3f578d326a470317eec72be531e70898c39984 translatedAt=2026-10-15T02:33:37.786Z pushedAt=2026-10-16T10:53:07.313Z -->

## Applicable Products

Supported on all chips.

## Description

Queries a single field in the platform resource group.

## Prototype

```python
get_platform_resource(group: str, key: str) -> str
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **group** | Input | Non-empty platform resource group name. |
| **key** | Input | Non-empty field name. |

## Constraints

It can only be called within the current synchronous `compile` callback.

## Examples

```python
resource = platform.get_platform_resource("ai_core", "count")
```
