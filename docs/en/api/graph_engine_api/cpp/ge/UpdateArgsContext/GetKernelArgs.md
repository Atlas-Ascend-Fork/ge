# GetKernelArgs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-26T13:36:05.240Z pushedAt=2026-10-28T07:42:19.877Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <graph/custom\_op.h\>
- Library file: liblowering.so

## Description

Obtains the kernel args at the specified placement and index.

## Prototype

```c++
const KernelArgs* GetKernelArgs(Placement placement, size_t index) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| placement | Input | **kPlacementHost** obtains host args, and **kPlacementDevice** obtains device args. |
| index | Input | Args index, corresponding to the call order of **MallocReadOnlyDevArgs** (the first malloc corresponds to index 0, and subsequent ones increment in order). |

## Return Value

**KernelArgs** pointer.

## Constraints

None

## Examples

```c++
graphStatus UpdateHostArgs(gert::UpdateArgsContext *ctx) override {
  // Obtain the latest addresses of the input and output tensors.
  const gert::Tensor *input_x = ctx->GetInputTensor(kInputX);
  const gert::Tensor *input_y = ctx->GetInputTensor(kInputY);
  const gert::Tensor *output_z = ctx->GetOutputTensor(kOutputZ);

  if (input_x == nullptr || input_y == nullptr || output_z == nullptr) {
    LOG_ERROR("UpdateHostArgs failed, input_x=", input_x, " input_y=", input_y, " output_z=", output_z);
    return GRAPH_FAILED;
  }

  // Obtain the host-side args pointer.
  const gert::KernelArgs *host_args = ctx->GetKernelArgs(gert::Placement::kPlacementHost, 0);
  if (host_args == nullptr) {
    LOG_ERROR("UpdateHostArgs failed, host_args is null");
    return GRAPH_FAILED;
  }

  // Refresh only the address fields in args.
  auto *args = static_cast<AddArgs *>(host_args->args_data);
  args->x_ptr = input_x->GetAddr();
  args->y_ptr = input_y->GetAddr();
  args->z_ptr = const_cast<void *>(output_z->GetAddr());

  return GRAPH_SUCCESS;
}
```
