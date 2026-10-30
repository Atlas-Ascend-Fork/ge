# get\_output\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:09:43.229Z pushedAt=2026-10-16T10:53:07.454Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the output attribute.

## Prototype

```python
get_output_attr(attr_name: str, output_index: int) -> Any
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name, of the string type. |
| output_index | Input | Output index. |

## Return Value

Returns the attribute value.

## Constraints

- If the parameter type is incorrect, a **TypeError** is thrown.
- If the retrieval fails, a **RuntimeError** is thrown.
