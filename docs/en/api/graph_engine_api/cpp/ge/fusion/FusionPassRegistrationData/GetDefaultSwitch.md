# GetDefaultSwitch

<!-- md-trans-meta sourceCommit=351a747bcd465b5e5848654d26a0978a5d08a8cd translatedAt=2026-10-16T13:25:25.850Z pushedAt=2026-10-28T07:40:22.541Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/fusion\_pass\_reg.h\>
- Library file: libge\_compiler.so

## Description

Obtains the default switch state of the fusion pass.

## Prototype

```c++
PassSwitch GetDefaultSwitch() const
```

## Parameters

None

## Return Value

Returns the default switch state of the pass, of type [PassSwitch](../../PassSwitch.md). When impl is null, returns `PassSwitch::kOn`.

## Constraints

None

## Examples

```c++
FusionPassRegistrationData pass_data("MyFusionPass");
pass_data.DefaultSwitch(PassSwitch::kOff);

// Read the default switch state.
PassSwitch switch_status = pass_data.GetDefaultSwitch();
// switch_status == PassSwitch::kOff
```
