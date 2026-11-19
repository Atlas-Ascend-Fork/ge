# GetStreamAllocationSummary

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:23:56.839Z pushedAt=2026-10-28T07:40:22.291Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Obtains the stream allocation summary.

## Prototype

```c++
Status GetStreamAllocationSummary(std::shared_ptr<StreamAllocationSummary> &stream_allocation) const
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| stream_allocation | Output | Stream allocation summary. |

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | Status | - **SUCCESS**: API call succeeded.<br>  - **FAILED**: API call failed. |

## Constraints

None
