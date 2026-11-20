# LogicalStreamAllocationInfo Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T14:45:07.045Z pushedAt=2026-10-28T07:40:22.856Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/ge\_graph\_compile\_summary.h\>
- Library file: libge\_compiler.so

## Description

Constructor and destructor of **LogicalStreamAllocationInfo**.

## Prototype

```c++
LogicalStreamAllocationInfo()
~LogicalStreamAllocationInfo()
LogicalStreamAllocationInfo(const LogicalStreamAllocationInfo &stream_info)
LogicalStreamAllocationInfo &operator=(const LogicalStreamAllocationInfo &stream_info)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| stream_info | Input | Stream information. |

## Return Value

None

## Constraints

None
