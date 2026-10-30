# AnyTypeOperator

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:30:56.993Z pushedAt=2026-10-28T07:40:22.322Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

An arbitrary type operator class that inherits from **Operator** and provides access to IR-related methods in the **Operator** class, including dynamic input/output registration, input/output registration, attribute registration, and subgraph registration.

## Prototype

```c++
AnyTypeOperator(const char_t *name, const char_t *type)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Operator name. |
| type | Input | Operator type. |

## Return Value

The **AnyTypeOperator** constructor returns an object of the **AnyTypeOperator** type.

## Constraints

None
