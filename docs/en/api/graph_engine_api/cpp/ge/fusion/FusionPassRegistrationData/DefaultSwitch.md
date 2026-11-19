# DefaultSwitch

<!-- md-trans-meta sourceCommit=351a747bcd465b5e5848654d26a0978a5d08a8cd translatedAt=2026-10-16T13:23:46.364Z pushedAt=2026-10-28T07:40:22.535Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pass/fusion\_pass\_reg.h\>
- Library file: libge\_compiler.so

## Description

Sets the default switch state of a fusion pass. A pass declared as `kOff` is not executed by default and runs only when explicitly enabled by the user through runtime configuration.

## Prototype

```c++
FusionPassRegistrationData &DefaultSwitch(PassSwitch pass_switch)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| pass_switch | Input | Default switch state of the pass. For details, see [PassSwitch](../../PassSwitch.md). |

## Return Value

Returns a **FusionPassRegistrationData** object, which supports chained calls.

## Constraints

- If `DefaultSwitch` is not called, the default is `PassSwitch::kOn`, which is consistent with the historical behavior.
- If the `.so` needs to run on an earlier GE version (< 9.3.0), use the `COMPILER_VERSION_NUM` compilation macro to guard the `DefaultSwitch` call.

## Examples

```c++
// Register a fusion pass and declare it disabled by default.
REG_FUSION_PASS(RiskyFusionPass)
    .DefaultSwitch(PassSwitch::kOff)
    .Stage(CustomPassStage::kBeforeInferShape);

// Defaults to kOn when DefaultSwitch is not called (backward compatible).
REG_FUSION_PASS(SafeFusionPass)
    .Stage(CustomPassStage::kBeforeInferShape);
```
