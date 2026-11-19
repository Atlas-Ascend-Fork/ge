# set\_input\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:12:32.992Z pushedAt=2026-10-16T10:53:07.466Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the input attribute.

## Prototype

```python
set_input_attr(attr_name: str, input_index: int, value: Any) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name, of the string type. |
| input_index | Input | Input index. |
| value | Input | Attribute value. For the supported attribute value types, see [AttrValueType](l../../../../AttrValueType.md). |

## Return Value

None

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the setting fails, a **RuntimeError** is thrown.
