# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:34:52.752Z pushedAt=2026-10-28T07:42:19.869Z nl=2 -->

Context object for logical stream allocation Pass.

## Header File to Include

```c++
#include <register/register_custom_pass.h>
```

## Public Member Functions

```c++
explicit StreamPassContext(int64_t current_max_stream_id)
~StreamPassContext() override = default
int64_t AllocateNextStreamId()
int64_t GetCurrMaxStreamId() const
int64_t GetStreamId(const GNode &node) const
graphStatus SetStreamId(const GNode &node, int64_t stream_id)
```
