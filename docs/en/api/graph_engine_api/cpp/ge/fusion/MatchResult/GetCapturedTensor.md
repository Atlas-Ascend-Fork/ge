# GetCapturedTensor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:28:06.721Z pushedAt=2026-10-28T07:40:22.556Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/match\_result.h\>
- Library file: libge\_compiler.so

## Description

Obtains the corresponding **Node output** in the match based on the index captured in the **Pattern**.

## Prototype

```c++
Status GetCapturedTensor(size_t capture_idx, NodeIo &node_output) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| capture_idx | Input | Captured index ID. |
| node_output | Output | Matched Tensor. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | **SUCCESS**: Success.<br>**FAILED**: Failure. |

## Constraints

None
