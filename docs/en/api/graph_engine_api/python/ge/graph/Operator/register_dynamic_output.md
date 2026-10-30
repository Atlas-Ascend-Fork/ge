# register\_dynamic\_output

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:16:07.208Z pushedAt=2026-10-16T10:53:07.484Z -->

## Applicable Products

Supported on all chips.

## Description

Registers a dynamic output port group for an operator, declaring the port group name and the instance count of the current node. Dynamic outputs are used for operators with a variable number of outputs, and the parser completes the connections based on the instance count.

## Prototype

```python
Operator.register_dynamic_output(name: str, count: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| name | Input | Name of the dynamic output port group, of a non-empty string type. |
| count | Input | Number of dynamic output instances of the current node, of a non-negative integer type, with a value range of [0, 2^32-1]. |

## Return Value

None

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The source operator is a read-only object. Calling it throws `RuntimeError`.
- Dynamic port registration registers the number of instances required by the current node and does not change the port definition in the operator prototype.
- If `name` is not a non-empty string, `TypeError` is thrown.
- If `count` is not an integer, `TypeError` is thrown; if `count` is negative or out of range, `ValueError` is thrown.

## Examples

```python
target.register_dynamic_output("y", count)
```
