# Overview

<!-- md-trans-meta sourceCommit=a83d79e16734bcb373aa008e5d0dce501e2db450 translatedAt=2026-10-26T13:10:10.278Z pushedAt=2026-10-28T07:42:19.783Z nl=2 -->

Custom operators support the offline inference scenario APIs, which are used to serialize and deserialize custom operator data.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus Serialize(std::vector<uint8_t> &buffer) = 0
virtual graphStatus Deserialize(const std::vector<uint8_t> &buffer) = 0
```
