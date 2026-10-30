# StreamPassContext Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:35:38.094Z pushedAt=2026-10-28T07:42:19.873Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

**StreamPassContext** constructor and destructor.

## Prototype

```c++
explicit StreamPassContext(int64_t current_max_stream_id)
~StreamPassContext() override = default
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| current_max_stream_id | Input | Maximum Stream ID in the current graph. |

## Return Value

None

## Constraints

None
