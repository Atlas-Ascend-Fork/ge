# attr\_scope

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:41:00.031Z pushedAt=2026-10-16T10:53:07.345Z -->

## Applicable Products

Supported on all chips.

## Description

Attribute scope context manager.

## Prototype

```python
attr_scope(attr_maps)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **attr_maps** | Input | Attribute mapping dictionary. |

## Return Value

None

## Constraints

None

## Examples

```python
with attr_scope({"attr_name": attr_value}):
    tensor = builder.create_input(0)
```
