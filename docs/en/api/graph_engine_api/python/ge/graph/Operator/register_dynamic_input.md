# register\_dynamic\_input

<!-- md-trans-meta sourceCommit=500eefa22ce54caa25b8e15e68d19940c2524ed6 translatedAt=2026-10-15T03:16:08.086Z pushedAt=2026-10-16T10:53:07.486Z -->

## Applicable Products

Supported on all chips.

## Description

Registers a dynamic input port group for an operator, declaring the port group name and the instance count of the current node. Dynamic inputs are used for operators with a variable number of inputs (such as `Sum`), and the parser completes the connections based on the instance count.

## Prototype

```python
Operator.register_dynamic_input(name: str, count: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| name | Input | Name of the dynamic input port group, of a non-empty string type. |
| count | Input | Number of dynamic input instances of the current node, of a non-negative integer type, with a value range of [0, 2^32-1]. |

## Return Value

None

## Constraints

- Available only during callback execution. Calling it after the callback ends throws `RuntimeError`.
- The source operator is a read-only object. Calling it throws `RuntimeError`.
- The dynamic port registers the instance count required by the current node, without changing the port definition in the operator prototype.
- If `name` is not a non-empty string, `TypeError` is thrown.
- If `count` is not an integer, `TypeError` is thrown; if `count` is negative or out of the valid range, `ValueError` is thrown.

## Examples

```python
target.register_dynamic_input("x", count)
```

For a complete example of registering dynamic inputs based on the number of inputs of an ONNX node and recording the count attribute, see [`parse_node`](../../onnx_plugin/OnnxPlugin/parse_node.md).
