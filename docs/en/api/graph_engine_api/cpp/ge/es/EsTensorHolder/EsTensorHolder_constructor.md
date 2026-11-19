# EsTensorHolder Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:10:43.650Z pushedAt=2026-10-28T07:40:22.483Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

**EsTensorHolder** constructor.

## Prototype

```c++
EsTensorHolder(EsCTensorHolder *tensor) : tensor_holder_(tensor) {}
EsTensorHolder() = default
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | **EsCTensorHolder** pointer, pointing to the underlying C implementation object. |

## Return Value

The **EsTensorHolder** constructor returns an object of type **EsTensorHolder**.

## Constraints

None
