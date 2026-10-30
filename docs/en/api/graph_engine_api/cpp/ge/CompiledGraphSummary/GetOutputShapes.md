# GetOutputShapes

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:23:08.520Z pushedAt=2026-10-28T07:40:22.289Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

This API applies only to static shape graphs. It obtains the output shapes of the entire compiled graph, which can be used to calculate the memory size occupied by the outputs. For dynamic multi-profile graphs, it obtains the output shapes of the current maximum profile.

## Prototype

```c++
Status GetOutputShapes(std::vector<ge::Shape> &shapes) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| shapes | Output | A vector of the `ge::Shape` type. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: The API is called successfully.<br>  - **FAILED**: The API call fails. |

## Constraints

None
