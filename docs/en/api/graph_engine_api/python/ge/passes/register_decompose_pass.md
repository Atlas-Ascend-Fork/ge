# register\_decompose\_pass

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:50:34.102Z pushedAt=2026-10-16T10:53:07.596Z -->

## Applicable Products

Supported on all chips.

## Description

Registers the decorated **DecomposePass** subclass into the **Pass** registry and sets op\_types as an attribute of the class.

## Prototype

```python
register_decompose_pass(*, name: str, stage: PassStage, op_types: Iterable[str]) -> callable
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Pass name, of string type. It must be unique and cannot duplicate the name of an already registered pass. |
| stage | Input | Pass execution stage, of the **PassStage** enumeration type. |
| op_types | Input | List of operator types to be decomposed, of an iterable of strings. It cannot be empty, and each element must be a non-empty string. |

## Return Value

Returns a class decorator function. The decorated class is registered in the Pass registry, and op\_types is set as an attribute of the class.

## Constraints

None
