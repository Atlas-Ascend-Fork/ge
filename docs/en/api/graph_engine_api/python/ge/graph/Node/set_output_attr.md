# set\_output\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:12:36.320Z pushedAt=2026-10-16T10:53:07.468Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the output attribute.

## Prototype

```python
set_output_attr(attr_name: str, output_index: int, value: Any) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name. |
| output_index | Input | Output index. |
| value | Input | Attribute value. For the supported attribute value types, see [AttrValueType](../../AttrValueType.md). |

## Return Value

None

## Constraints

- If the parameter type is incorrect, **TypeError** is thrown.
- If the setting fails, **RuntimeError** is thrown.
