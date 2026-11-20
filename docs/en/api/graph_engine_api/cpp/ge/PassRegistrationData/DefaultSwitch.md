# DefaultSwitch

<!-- md-trans-meta sourceCommit=351a747bcd465b5e5848654d26a0978a5d08a8cd translatedAt=2026-10-26T13:06:21.800Z pushedAt=2026-10-28T07:42:19.767Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: libregister.so

## Description

Sets the default switch state of a custom Pass. A Pass declared as `kOff` is not executed by default; it is executed only when explicitly enabled by the user through runtime configuration.

## Prototype

```c++
PassRegistrationData &DefaultSwitch(PassSwitch pass_switch)
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| pass_switch | Input | Default switch state of the Pass. For details, see [PassSwitch](../PassSwitch.md). |

## Return Value

Returns the PassRegistrationData object, which supports chained calls.

## Constraints

- If `DefaultSwitch` is not called, the default is `PassSwitch::kOn`, which is consistent with the historical behavior.
- If the `.so` needs to run on an earlier version of GE (< 9.3.0), use the `COMPILER_VERSION_NUM` compilation macro to guard the `DefaultSwitch` call.

## Examples

```c++
// Register a custom Pass and declare it disabled by default.
REGISTER_CUSTOM_PASS("RiskyCustomPass")
    .DefaultSwitch(PassSwitch::kOff)
    .CustomPassFn(MyCustomPass)
    .Stage(CustomPassStage::kBeforeInferShape);

// Defaults to kOn when DefaultSwitch is not called (backward compatible).
REGISTER_CUSTOM_PASS("SafeCustomPass")
    .CustomPassFn(MyCustomPass)
    .Stage(CustomPassStage::kBeforeInferShape);
```
