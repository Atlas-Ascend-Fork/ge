# @pattern

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:00:42.305Z pushedAt=2026-10-16T10:53:07.646Z -->

## Applicable Products

Supported on all chips.

## Description

Defines a pattern using a Python expression. One **@pattern** method corresponds to one pattern, and multiple patterns can be declared with multiple **@pattern** methods.

## Prototype

```python
@pattern
def add_zero(self, inputs):
    return inputs[0] + 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **inputs** | Input | Set of expression pattern inputs. **inputs[i]** represents the i-th graph input; **inputs[:N]** explicitly declares multiple consecutive graph inputs. |

## Return Value

| Type | Description |
| --- | --- |
| **TensorHolder** | Returns a single-output pattern expression. |
| **list[TensorHolder]** / **tuple[TensorHolder, ...]** | Returns a multi-output pattern; the list or tuple does not represent multiple patterns. |

## Constraints

- The **@pattern** method only declares a pattern and does not accept match\_result.
- A multi-pattern pass is declared through multiple **@pattern** methods, not by returning multiple **Pattern**/**Graph** objects from a single method.
- The number of **inputs** is unknown, so they cannot be iterated directly. For multi-input scenarios, use inputs\[:N\].
- The Python layer automatically creates the **GraphBuilder**, graph inputs, and graph outputs, and automatically captures the visited **inputs** and the pattern outputs returned by **@pattern**.
- The automatic capture order is fixed: first capture the visited **inputs** in input index order, then capture the pattern outputs in the order of the **return** structure. When the same **Tensor** serves as both an input and an output, it is captured once for each of these two roles.

## Examples

```python
from ge.passes import PatternFusionPass, pattern


class AlgebraicPass(PatternFusionPass):
    @pattern
    def add_zero(self, inputs):
        return inputs[0] + 0

    @pattern
    def mul_one(self, inputs):
        return inputs[0] * 1

    def replacement(self, inputs):
        return inputs[0]
```

For more usage, see [Fusion Pass Examples](https://gitcode.com/cann/ge/tree/master/examples/fusion_pass).
