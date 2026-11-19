# GetDefaultSwitch

<!-- md-trans-meta sourceCommit=351a747bcd465b5e5848654d26a0978a5d08a8cd translatedAt=2026-10-26T13:07:36.499Z pushedAt=2026-10-28T07:42:19.771Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Obtains the default switch state of a custom Pass.

## Prototype

```c++
PassSwitch GetDefaultSwitch() const
```

## Parameters

None

## Return Value

Returns the default switch state of the Pass, of type [PassSwitch](../PassSwitch.md). Returns `PassSwitch::kOn` when impl is null.

## Constraints

None

## Examples

```c++
PassRegistrationData pass_data("MyCustomPass");
pass_data.DefaultSwitch(PassSwitch::kOff);

// Read the default switch state.
PassSwitch switch_status = pass_data.GetDefaultSwitch();
// switch_status == PassSwitch::kOff
```
