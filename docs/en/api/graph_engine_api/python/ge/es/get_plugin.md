# get\_plugin

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:41:25.512Z pushedAt=2026-10-16T10:53:07.348Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the plugin module with the specified name.

## Prototype

```python
get_plugin(plugin_name: str) -> Union[ModuleType, None]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **plugin_name** | Input | Plugin name (for example, 'math' or 'nn'). |

## Return Value

\(ModuleType | None\) Returns the plugin module object, or **None** if not found.

## Constraints

None
