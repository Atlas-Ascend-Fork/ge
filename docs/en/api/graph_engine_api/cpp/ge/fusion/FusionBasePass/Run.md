# Run

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:23:01.933Z pushedAt=2026-10-28T07:40:22.532Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/fusion\_base\_pass.h\>
- Library file: libge\_compiler.so

## Description

Main function for Pass execution.

## Prototype

```c++
virtual Status Run(GraphPtr &graph, CustomPassContext &pass_context) = 0
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| graph | Input | Target graph. |
| pass_context | Input | Custom pass context, which can be used to pass information such as error messages. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Execution succeeded.<br>**FAILED**: Execution failed. |

## Constraints

None
