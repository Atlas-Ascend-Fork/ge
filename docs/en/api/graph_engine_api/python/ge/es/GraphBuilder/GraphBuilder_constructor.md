# GraphBuilder Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:48:59.696Z pushedAt=2026-10-16T10:53:07.375Z -->

## Applicable Products

Supported on all chips.

## Description

Initializes a **GraphBuilder**.

Lifecycle notes:

- All **TensorHolder** objects created by this builder keep a reference to it.
- The builder is not garbage collected as long as any of its tensors are still referenced.
- After calling build\_and\_reset\(\), the builder enters the built state and can no longer be used to create new tensors.
- A new **GraphBuilder** is required to build another graph.

## Prototype

```python
GraphBuilder(name: Optional[str] = None)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Graph name, of string type, defaulting to "graph" if None. |

## Return Value

None

## Constraints

- If **name** is not a string or **None**, a **TypeError** is thrown.
- If the graph builder fails to be created, a **RuntimeError** is thrown.
