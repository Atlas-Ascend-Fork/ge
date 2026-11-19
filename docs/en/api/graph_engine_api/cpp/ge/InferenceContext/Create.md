# Create

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T14:28:38.899Z pushedAt=2026-10-28T07:40:22.790Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/inference\_context.h\>
- Library file: libgraph.so

## Description

Creates the context object of a resource operator in the inference context of resource operators.

## Prototype

```c++
static std::unique_ptr<InferenceContext> Create(
void *resource_context_mgr = nullptr
)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| resource_context_mgr | Input | Resource Context manager pointer.<br>This pointer is initialized when a Session is created, is automatically passed in by the InferShape framework, and has the same lifecycle as the Session. |

## Return Value

Context object passed between resource operators.

## Constraints

None
