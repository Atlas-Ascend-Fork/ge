# AddResource

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:51:53.396Z pushedAt=2026-10-28T07:40:22.412Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_c\_graph\_builder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Transfers the passed-in resource to **EsCGraphBuilder** for management.

## Prototype

```c++
T *AddResource(std::unique_ptr<T> value, std::function<void(void*)> deleter = DefaultDeleter<T>)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| value | Input | Resource to be transferred. |
| deleter | Input | Resource destructor. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | T * (type of the resource to be transferred) | Resource holder pointer. |

## Constraints

None
