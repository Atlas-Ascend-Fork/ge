# Overview

<!-- md-trans-meta sourceCommit=a83d79e16734bcb373aa008e5d0dce501e2db450 translatedAt=2026-10-26T13:10:10.278Z pushedAt=2026-10-28T07:42:19.783Z nl=2 -->

API for data serialization/deserialization of custom operators. After this API is implemented, GE calls back the Serialize API during model saving to serialize custom operator data (such as kernel bin) into a buffer and save it with the offline model (OM). During model loading, GE calls back the Deserialize API to restore the operator data from the buffer. The buffer format is user-defined; GE only passes it through without parsing it.

Custom operators that do not implement this API can also be used to generate offline models: operators that implement [AnnotatedArgsOp](../AnnotatedArgsOp/overview.md) can submit kernel data to GE through DeclareLaunchArgs at compile time, which is saved with the model and used for launch during model loading and execution. Otherwise, their kernel binary and other data are not saved or restored with the OM and must be managed by the user.

## Header File to Include

```c++
#include <graph/custom_op.h>
```

## Public Member Functions

```c++
virtual graphStatus Serialize(std::vector<uint8_t> &buffer) = 0
virtual graphStatus Deserialize(const std::vector<uint8_t> &buffer) = 0
```
