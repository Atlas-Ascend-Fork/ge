# meet\_requirements

<!-- md-trans-meta sourceCommit=57e742592fb0b093fa4f5d82c393c97b374a8bfd translatedAt=2026-10-15T04:00:09.797Z pushedAt=2026-10-16T10:53:07.640Z -->

## Applicable Products

Supported on all chips.

## Description

Determines whether the match result meets the replacement condition. This is an optional implementation and returns True by default.

## Prototype

```python
meet_requirements(self, match_result: MatchResult) -> bool
meet_requirements(self, match_result: MatchResult, context: PassContext) -> bool
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| match_result | Input | Pattern match result, of type **MatchResult**, containing the matched nodes and edge information. |
| context | Input | Current compilation-phase **PassContext**, valid only within the current callback stack, used to read compilation options or set error information. |

## Return Value

Returns **True** if the replacement condition is met, in which case the replacement is performed; returns **False** if the condition is not met, in which case this replacement is skipped. The default return value is **True**.

## Constraints

The `context` parameter can be optionally declared; when it is not declared, the original single-parameter calling method is retained.
