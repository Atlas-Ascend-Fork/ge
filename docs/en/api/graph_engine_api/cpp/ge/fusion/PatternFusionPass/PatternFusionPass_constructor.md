# PatternFusionPass Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:33:56.424Z pushedAt=2026-10-28T07:40:22.581Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/pattern\_fusion\_pass.h\>
- Library file: libge\_compiler.so

## Description

Constructor of **PatternFusionPass**.

For a pass created through this constructor, all Match Config options are disabled by default. Subclasses can pass in **PatternMatcherConfig** to enable the required matching options.

## Prototype

```c++
PatternFusionPass()
explicit PatternFusionPass(std::unique_ptr<PatternMatcherConfig> match_config)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| std::unique_ptr\<PatternMatcherConfig> match_config | Input | Match rule configuration. |

## Return Value

None

## Constraints

None
