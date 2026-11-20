# set\_error\_message

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:57:26.861Z pushedAt=2026-10-16T10:53:07.627Z -->

## Applicable Products

Supported on all chips.

## Description

Sets the error information for the Graph Engine to record or report.

## Prototype

```python
set_error_message(message: str) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **message** | Input | Error message to be set, of string type. |

## Return Value

None

## Constraints

It can be used only within the current run call stack.

## Examples

```python
from ge.passes import FusionBasePass, PassContext


class MyPass(FusionBasePass):
    def run(self, graph, context: PassContext):
        # self._ok is a user-defined graph validation method; set the error message when validation fails.
        if not self._ok(graph):
            context.set_error_message("my pass: invariant violated")
            return False
        return True

```
