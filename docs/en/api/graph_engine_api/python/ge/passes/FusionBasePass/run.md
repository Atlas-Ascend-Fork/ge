# run

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:53:26.288Z pushedAt=2026-10-16T10:53:07.611Z -->

## Applicable Products

Supported on all chips.

## Description

The entry point of a **Pass**, invoked by the engine at the corresponding compilation phase to execute optimization logic on the computation graph.

## Prototype

```python
run(graph: Graph, context: PassContext) -> StatusLike
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Graph object to be optimized, of type **ge.graph.Graph**. |
| context | Input | Pass execution context, of type **PassContext**, which provides information about the current compilation environment. |

## Return Value

Returns **None**, **bool**, or **int**. Returning **None** or a truthy value indicates successful execution; returning a falsy value **False** or **0** indicates execution failure.

## Constraints

Subclasses must implement this method.
