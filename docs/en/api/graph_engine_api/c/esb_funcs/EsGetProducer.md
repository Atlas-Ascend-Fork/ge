# EsGetProducer

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T16:23:19.460Z pushedAt=2026-10-28T13:23:14.317Z nl=2 -->

## Product Support

Full chip support.

## Header File/Library File

- Header file: \#include <ge/esb\_funcs.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Feature Description

This API returns a snapshot of the ge::GNode instance.

## Prototype

```c
EsCNode *EsGetProducer(EsCTensorHolder *tensor)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| tensor | Input | Tensor holder pointer. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | EsCNode * | Returns the node pointer on success, or nullptr on failure. |

## Constraints

None

## Examples

```c
auto *tensor = EsCreateGraphInput(graph, 0);
auto *producer = static_cast<void *>(EsGetProducer(tensor));
```
