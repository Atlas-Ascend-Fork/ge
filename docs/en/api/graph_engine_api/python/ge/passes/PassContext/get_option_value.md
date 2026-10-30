# get\_option\_value

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:56:04.984Z pushedAt=2026-10-16T10:53:07.621Z -->

## Applicable Products

Supported on all chips.

## Description

Returns the string value of the compilation option corresponding to **key**.

## Prototype

```python
get_option_value(key: str) -> str
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| key | Input | Compilation option key name, of string type. |

## Return Value

Returns the compilation option string value corresponding to **key**.

## Constraints

- **key** must exist and be readable; otherwise, a **RuntimeError** is thrown.
- It can only be used within the current run call stack.

## Examples

```python
from ge.passes import FusionBasePass, PassContext


class MyPass(FusionBasePass):
    def run(self, graph, context: PassContext):
        # Read the compilation option in run().
        opt = context.get_option_value("some.option.key")
        return True
```

For more usage, see [Fusion Pass Examples](https://gitcode.com/cann/ge/tree/master/examples/fusion_pass).
