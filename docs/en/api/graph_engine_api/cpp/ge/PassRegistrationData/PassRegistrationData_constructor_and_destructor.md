# PassRegistrationData Constructor and Destructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:09:04.951Z pushedAt=2026-10-28T07:42:19.778Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Constructor and destructor of the PassRegistrationData class.

## Prototype

```c++
PassRegistrationData() = default
~PassRegistrationData() = default
PassRegistrationData(std::string pass_name)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| pass_name | Input | Name of the custom Pass. |

## Return Value

The PassRegistrationData constructor returns an object of the PassRegistrationData type.

## Constraints

None
