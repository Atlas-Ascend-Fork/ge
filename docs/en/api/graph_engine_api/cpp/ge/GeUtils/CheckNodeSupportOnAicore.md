# CheckNodeSupportOnAicore

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:57:48.623Z pushedAt=2026-10-28T07:40:22.673Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_utils.h\>
- Library file: libge\_compiler.so

## Description

Validates the passed-in **Node** to check whether it supports execution on the **AI Core**.

## Prototype

```c++
static Status CheckNodeSupportOnAicore(const GNode &node, bool &is_supported, AscendString &unsupported_reason)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node to be checked. |
| is_supported | Output | Whether execution on the AI Core is supported.<br><br>  - **true**: Supported.<br>  - **false**: Not supported. |
| unsupported_reason | Output | Specific reason for not being supported. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | graphStatus | **SUCCESS**: Check succeeded.<br>**FAILED**: Check failed. |

## Constraints

Before calling this API, ensure that the shape of the node has been inferred, because the validation process requires shape information for judgment.
