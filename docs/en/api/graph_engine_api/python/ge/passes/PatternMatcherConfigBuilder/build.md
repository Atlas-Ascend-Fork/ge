# build

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:03:55.655Z pushedAt=2026-10-16T10:53:07.658Z -->

## Applicable Products

Supported on all chips.

## Description

Constructs and returns a **PatternMatcherConfig** instance.

## Prototype

```python
build() -> PatternMatcherConfig
```

## Parameters

None

## Return Value

Returns the constructed **PatternMatcherConfig** instance.

## Constraints

If the C++ side fails to build, a **RuntimeError** is thrown.

## Example

```python
from ge.passes import PatternFusionPass, PatternMatcherConfigBuilder


class TestPass(PatternFusionPass):
    def __init__(self):
        super().__init__(
            PatternMatcherConfigBuilder()
            .enable_const_value_match()
            .enable_ir_attr_match()
            .build()
        )
```
