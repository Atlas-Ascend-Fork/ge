# get\_error\_message

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:55:16.744Z pushedAt=2026-10-16T10:53:07.620Z -->

## Applicable Products

Supported on all chips.

## Description

Returns the error message set by the engine or a **Pass** (an empty string is returned if no error message is set).

## Prototype

```python
get_error_message() -> str
```

## Parameters

None

## Return Value

Returns the error message string; returns an empty string if no error message is set.

## Constraints

It can only be used within the call stack of the current run.

## Examples

```python
from ge.passes import FusionBasePass, PassContext


class MyPass(FusionBasePass):
    def run(self, graph, context: PassContext):
        # Read the error information in run().
        msg = context.get_error_message()
        return True
```
