# register\_output

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:17:27.025Z pushedAt=2026-10-16T10:53:07.489Z -->

## Applicable Products

Supported on all chips.

## Description

Registers an output port for an operator to supplement the operator's output port definition during the parsing phase.

## Prototype

```python
Operator.register_output(name: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **name** | Input | Output port name, of non-empty string type. |

## Return Value

None

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The source operator is a read-only object. Calling it throws `RuntimeError`.
- Ports already declared in the target operator prototype should not be registered repeatedly. Instance ports of dynamic input/output prototypes (such as `PartitionedCall`) must be registered through this method; otherwise, the parser fails during the wiring phase.
- If `name` is not a non-empty string, `TypeError` is thrown.

## Examples

```python
target.register_output("y")
```
