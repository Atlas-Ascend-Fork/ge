# register\_optional\_input

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:17:09.178Z pushedAt=2026-10-16T10:53:07.488Z -->

## Applicable Products

Supported on all chips.

## Description

Registers an optional input port for an operator to supplement the operator's optional input port definition during the parsing phase.

## Prototype

```python
Operator.register_optional_input(name: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **name** | Input | Name of the optional input port, of non-empty string type. |

## Return Value

None

## Constraints

- Only available during callback execution. Calling it after the callback ends raises `RuntimeError`.
- The **source** operator is a read-only object. Calling it raises `RuntimeError`.
- Ports already declared in the target operator prototype should not be registered again.
- Optional input instance ports of dynamic input/output prototypes (such as `PartitionedCall`) must be registered through this method; otherwise, the parser wiring phase fails.
- If `name` is not a non-empty string, `TypeError` is raised.

## Examples

```python
target.register_optional_input("bias")
```
