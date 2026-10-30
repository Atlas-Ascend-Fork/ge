# InputAddr

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:53:13.864Z pushedAt=2026-10-28T08:50:45.410Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

`gert::InputAddr` is a logical input address descriptor. As a parameter of [`AnnotatedKernelArgs::AppendArg`](AnnotatedKernelArgs/AppendArg.md), it marks a parameter in the kernel arguments as an operator input address.

## Prototype

```c++
namespace gert {
struct InputAddr {
  uint32_t index;    // Input index in the operator instance.
  const void *addr;  // Input logical address.
};
}  // namespace gert
```

## Parameters

| Name | Type | Description |
| --- | --- | --- |
| `index` | `uint32_t` | Input index in the operator instance. The value must not exceed `INT32_MAX`. |
| `addr` | `const void *` | Input logical address appended to the args parameter. |

## Return Value

None

## Constraints

- `addr` should come from the input Tensor available in the current compile-time callback, for example, the address of the return value of [`AnnotatedArgsContext::GetInputTensor`](AnnotatedArgsContext/GetInputTensor.md).

## Examples

```c++
const auto *input = ctx.GetInputTensor(0U);
if (input == nullptr) {
  return ge::GRAPH_FAILED;
}
gert::AnnotatedKernelArgs args(
    gert::InputAddr{0U, input->GetAddr()});
return ctx.AddLaunch(launch_info, std::move(args));
```
