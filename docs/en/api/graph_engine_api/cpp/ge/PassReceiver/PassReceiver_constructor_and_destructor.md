# PassReceiver Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:00:49.284Z pushedAt=2026-10-28T07:42:19.763Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Constructor and destructor of **PassReceiver**.

## Prototype

```c++
PassReceiver(PassRegistrationData &reg_data)
~PassReceiver() = default
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| reg_data | Input | Pass information to be registered. |

## Return Value

The **PassReceiver** constructor returns an object of type **PassReceiver**.

## Constraints

None
