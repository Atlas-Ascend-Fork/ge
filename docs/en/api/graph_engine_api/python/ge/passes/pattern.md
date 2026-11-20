# pattern

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:49:56.930Z pushedAt=2026-10-16T10:53:07.595Z -->

## Applicable Products

Supported on all chips.

## Description

A decorator that marks a method of **PatternFusionPass** as an expression pattern declaration.

## Prototype

```python
pattern(method: Callable[..., object]) -> Callable[..., object]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| method | Input | Instance method in a **PatternFusionPass** subclass, with the signature **method(self, inputs)**. |

## Return Value

Returns the original method object, which is collected as a pattern by **PatternFusionPass** during the class definition phase.

## Constraints

None
