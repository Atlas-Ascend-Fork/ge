# get\_pass\_name

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:56:07.522Z pushedAt=2026-10-16T10:53:07.623Z -->

## Applicable Products

Supported on all chips.

## Description

Returns the name of the current **Pass**.

## Prototype

```python
get_pass_name() -> str
```

## Parameters

None

## Return Value

Returns the name of the current **Pass**, which is a string.

## Constraints

It can be used only within the current **run** call stack (or other synchronous callbacks defined by the engine). Do not save it to **self** for use by other threads.

## Examples

```python
from ge.passes import FusionBasePass, PassContext


class MyPass(FusionBasePass):
    def run(self, graph, context: PassContext):
        # Obtain the current pass name in run().
        name = context.get_pass_name()
        return True
```

For more usage, see [Fusion Pass Examples](https://gitcode.com/cann/ge/tree/master/examples/fusion_pass).
