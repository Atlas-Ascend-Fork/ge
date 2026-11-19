# OutputAddr

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:53:10.420Z pushedAt=2026-10-28T08:50:45.408Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

`gert::OutputAddr` is a logical output address descriptor. As a parameter of [`AnnotatedKernelArgs::AppendArg`](AnnotatedKernelArgs/AppendArg.md), it marks a parameter in kernel args as the operator output address.

## Prototype

```c++
namespace gert {
struct OutputAddr {
  uint32_t index;    // Output index in the operator instance.
  const void *addr;  // Output logical address.
};
}  // namespace gert
```

## Parameters

| Name | Type | Description |
| --- | --- | --- |
| `index` | `uint32_t` | Output index in the operator instance. The value must not be greater than `INT32_MAX`. |
| `addr` | `const void *` | Output logical address appended to the args parameter. |

## Return Value

None

## Constraints

- `addr` should come from the output tensor available in the current compile-time callback, for example, the address of the return value of [`AnnotatedArgsContext::GetOutputTensor`](AnnotatedArgsContext/GetOutputTensor.md).

## Examples

```c++
const auto *output = ctx.GetOutputTensor(0U);
if (output == nullptr) {
  return ge::GRAPH_FAILED;
}
gert::AnnotatedKernelArgs args(
    gert::OutputAddr{0U, output->GetAddr()});
return ctx.AddLaunch(launch_info, std::move(args));
```
