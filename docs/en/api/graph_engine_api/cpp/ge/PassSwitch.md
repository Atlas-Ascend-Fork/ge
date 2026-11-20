# PassSwitch

<!-- md-trans-meta sourceCommit=351a747bcd465b5e5848654d26a0978a5d08a8cd translatedAt=2026-10-16T11:24:06.784Z pushedAt=2026-10-28T07:40:22.171Z nl=2 -->

Enum of the default switch states of passes. Header file: \#include <register/register\_custom\_pass.h\>

```c++
enum class PassSwitch : uint32_t {
  kOn = 0,
  kOff = 1
};
```

- **kOn**: The pass is enabled by default (consistent with historical behavior). This is the default value when `DefaultSwitch` is not called.
- **kOff**: The pass is disabled by default. It is executed only when the user explicitly enables it through the `fusion_switch_file` JSON configuration or the `--optimization_switch` command-line parameter.

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <register/register\_custom\_pass.h\>
- Library file: **libregister.so**

## Constraints

None

## Notes

`kOff` applies to passes that may introduce precision risks or performance regressions. When declared as `kOff`, the pass is not executed by default at registration. Users can override the registered default value through runtime configuration (graph option > JSON exact match > JSON ALL wildcard) to explicitly enable the pass.

## Examples

```c++
// Declare that the pass is disabled by default.
REGISTER_CUSTOM_PASS("RiskyCustomPass")
    .DefaultSwitch(PassSwitch::kOff)
    .Stage(CustomPassStage::kBeforeInferShape);

// Defaults to kOn when DefaultSwitch is not called.
REGISTER_CUSTOM_PASS("SafeCustomPass")
    .Stage(CustomPassStage::kBeforeInferShape);
```
