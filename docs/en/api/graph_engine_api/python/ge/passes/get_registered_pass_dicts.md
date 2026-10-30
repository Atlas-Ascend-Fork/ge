# get\_registered\_pass\_dicts

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:48:04.407Z pushedAt=2026-10-16T10:53:07.588Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the list of dictionary representations of all registered **Pass** objects.

## Prototype

```python
get_registered_pass_dicts() -> List[dict]
```

## Parameters

None

## Return Value

| Type | Description |
| --- | --- |
| List[dict] | Returns a list of dictionaries of registered passes. Each dictionary contains fields such as descriptor_key, pass_name, module_name, class_name, stage, kind, and op_types. |

## Constraints

None
