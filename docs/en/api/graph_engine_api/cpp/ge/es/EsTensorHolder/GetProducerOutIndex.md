# GetProducerOutIndex

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:12:07.393Z pushedAt=2026-10-28T07:40:22.487Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/es\_tensor\_holder.h\>
- Library file: libeager\_style\_graph\_builder\_base.so, libeager\_style\_graph\_builder\_base\_static.a

## Description

Obtains the output index.

## Prototype

```c++
int32_t GetProducerOutIndex() const
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | int32_t | Output index. |

## Constraints

None

## Examples

```c++
  auto builder = std::make_unique<ge::EsGraphBuilder>("graph");
  auto tensor0 = builder->CreateInput(0);
  int32_t outputIndex = tensor0.GetProducerOutIndex(); // Obtain the output index of output tensor0.
```
