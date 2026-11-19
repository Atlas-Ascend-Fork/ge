# meet\_requirements

<!-- md-trans-meta sourceCommit=57e742592fb0b093fa4f5d82c393c97b374a8bfd translatedAt=2026-10-15T03:51:40.444Z pushedAt=2026-10-16T10:53:07.603Z -->

## Applicable Products

Supported on all chips.

## Description

Determines whether a node needs to be decomposed. This is an optional implementation and returns **True** by default.

## Prototype

```python
meet_requirements(self, node: Node) -> bool
meet_requirements(self, node: Node, context: PassContext) -> bool
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node to be determined, of type **ge.graph.Node**. |
| context | Input | Current compile-time **PassContext**, valid only within the current callback stack. It can read compilation options or set error information. |

## Return Value

Returns **True** to indicate that decomposition is required and replacement will be performed; returns **False** to indicate that decomposition is not required and the node is skipped. **True** is returned by default.

## Constraints

Optionally declare the `context` parameter; if not declared, the original single-parameter calling method is retained.
