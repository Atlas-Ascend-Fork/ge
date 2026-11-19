# patterns

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:01:42.031Z pushedAt=2026-10-16T10:53:07.647Z -->

## Applicable Products

Supported on all chips.

## Description

Defines the list of patterns to be matched. It serves as the explicit graph construction entry for legacy and is suitable for directly returning one or more Pattern/Graph objects.

## Prototype

```python
patterns(self) -> Iterable[PatternOrGraph]
```

## Parameters

None

## Return Value

Returns an iterable object, where each element is of the **Pattern** or **Graph** type, representing the subgraph pattern to be matched.

## Constraints

- Cannot be used together with the **@pattern** method.
- The patterns\(self, inputs\) syntax is not supported.
