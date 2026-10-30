# register\_input

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:16:04.330Z pushedAt=2026-10-16T10:53:07.479Z -->

## Applicable Products

Supported on all chips.

## Description

Registers a required input port for an operator, which is used to supplement the operator's input port definition during the parsing phase.

## Prototype

```python
Operator.register_input(name: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **name** | Input | Input port name, of the non-empty string type. |

## Return Value

None

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The **source** operator is a read-only object. Calling it throws `RuntimeError`.
- Ports already declared in the target operator prototype should not be registered repeatedly. Instance ports of dynamic input/output prototypes (such as `PartitionedCall`) must be registered through this method; otherwise, the parser fails during the wiring phase.
- If `name` is not a non-empty string, `TypeError` is thrown.

## Examples

```python
target.register_input("x")
```
