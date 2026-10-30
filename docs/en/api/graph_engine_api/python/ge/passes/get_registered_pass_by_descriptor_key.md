# get\_registered\_pass\_by\_descriptor\_key

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:47:58.725Z pushedAt=2026-10-16T10:53:07.586Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the registered Pass descriptor based on the descriptor key.

## Prototype

```python
get_registered_pass_by_descriptor_key(descriptor_key: str) -> Optional[PassDescriptor]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| descriptor_key | Input | Pass descriptor key, of string type, in the format {module_name}:{class_name}:{pass_name}. |

## Return Value

| Type | Description |
| --- | --- |
| Optional[PassDescriptor] | Returns the matching **PassDescriptor** object; returns **None** if no match is found. |

## Constraints

None
