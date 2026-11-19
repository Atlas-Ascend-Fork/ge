# Constructors and Destructors

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T12:36:21.410Z pushedAt=2026-10-28T07:42:19.734Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/op\_lib\_register.h\>
- Library file: libregister.so

## Description

Constructor and destructor of OpLibRegister.

## Prototype

```c++
explicit OpLibRegister(const char_t *vendor_name)
OpLibRegister(OpLibRegister &&other) noexcept
OpLibRegister(const OpLibRegister &other)
OpLibRegister &operator=(const OpLibRegister &) = delete
OpLibRegister &operator=(OpLibRegister &&) = delete
~OpLibRegister()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| vendor_name | Input | Name of the custom operator vendor. |
| other | Input | Another OpLibRegister object. |

## Return Value

Returns an OpLibRegister object.

## Constraints

None
