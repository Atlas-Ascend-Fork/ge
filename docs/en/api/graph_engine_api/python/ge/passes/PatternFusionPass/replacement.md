# replacement

<!-- md-trans-meta sourceCommit=57e742592fb0b093fa4f5d82c393c97b374a8bfd translatedAt=2026-10-15T04:02:02.608Z pushedAt=2026-10-16T10:53:07.650Z -->

## Applicable Products

Supported on all chips.

## Description

Generates a replacement subgraph.

## Prototype

```python
replacement(self, match_result: MatchResult) -> Graph
replacement(self, match_result: MatchResult, context: PassContext) -> Graph
```

The expression pattern can also be written as follows:

```python
# Simple replacement that does not require match details.
def replacement(self, inputs) -> TensorHolder: ...
# Read captured tensors or node attributes.
def replacement(self, inputs, match_result) -> TensorHolder: ...
# Read the compile-time context.
def replacement(self, inputs, context: PassContext) -> TensorHolder: ...
# Read both match details and the compile-time context.
def replacement(self, inputs, match_result, context: PassContext) -> TensorHolder: ...
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| self | Input | Pass instance object of the **PatternFusionPass** subclass. Used to access pass configuration and call methods. |
| inputs | Input | Input set of the replacement graph of the **PatternInputs** type, used to create input tensors of the replacement graph. |
| match_result | Input | Pattern matching result of the **MatchResult** type, containing the matched node and edge information. |
| context | Input | Current compile-time **PassContext**, valid only within the current callback stack, used to read compilation options or set error information. |

## Return Value

| Type | Description |
| --- | --- |
| Graph | Returns the replacement subgraph, of the type **ge.graph.Graph**. |

## Constraints

The **expression replacement\(self, inputs\)** can return a **TensorHolder** or a non-empty list/tuple of **TensorHolder** objects, and the Python layer automatically constructs the replacement graph. When match details need to be read, a **match\_result** parameter can be added. When only the compile-time context is needed, **replacement\(self, inputs, context\)** can be used, where the third parameter must be named `context`. When both match details and the compile-time context need to be read, **replacement\(self, inputs, match\_result, context\)** can be used, where the fourth parameter must be named `context`. When `context` is not declared, the original calling convention is retained.
