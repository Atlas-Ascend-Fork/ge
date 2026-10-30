# get\_input\_attr

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:07:46.864Z pushedAt=2026-10-16T10:53:07.447Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the input attribute.

## Prototype

```python
get_input_attr(attr_name: str, input_index: int) -> Any
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| attr_name | Input | Attribute name. |
| input_index | Input | Input index. |

## Return Value

Returns the attribute value.

## Constraints

- If the parameter type is incorrect, a TypeError is thrown.
- If the retrieval fails, a RuntimeError is thrown.
