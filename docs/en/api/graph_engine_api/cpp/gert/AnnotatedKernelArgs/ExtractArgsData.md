# ExtractArgsData

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T08:01:35.856Z pushedAt=2026-10-28T08:50:45.442Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Extracts the kernel parameter data and parameter descriptions already built in `gert::AnnotatedKernelArgs`. You can use it to check the build result, but you should not reorder or rewrite the descriptions.

## Prototype

```c++
ge::graphStatus ExtractArgsData(std::vector<uint8_t> &args_data,
                                std::vector<ge::ArgDesc> &arg_descs) const
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| `args_data` | Output | Byte data of the kernel args. The data order is the same as the append order. |
| `arg_descs` | Output | Array of `ge::ArgDesc` descriptions that correspond one-to-one to the parameters in `args_data`. For an address parameter, the description indicates the type and index of the input, output, or workspace; for a `uint64_t` parameter, the description indicates the custom value. |

On success, the original contents of the two vectors are overwritten by the results; on failure, do not use the output contents.

## Return Value

| Name | Type | Description |
| --- | --- | --- |
| - | `ge::graphStatus` | `GRAPH_SUCCESS(0)`: Extraction succeeded; other values: the object is empty or in an invalid state, a previous append failed, there are no parameters, or the number of parameter data entries does not match the number of descriptions. |

## Constraints

- At least one parameter must be successfully appended; extraction fails for an empty `AnnotatedKernelArgs`.
- All append operations must succeed, and `args_data.size()` must equal `arg_descs.size() * sizeof(uint64_t)`; any failure state causes extraction to fail.
- The `i`-th parameter containing `args_data[i]` strictly corresponds to `arg_descs[i]`.

## Examples

```c++
gert::AnnotatedKernelArgs args(
    gert::InputAddr{0U, input_addr}, uint64_t{7U});
std::vector<uint8_t> args_data;
std::vector<ge::ArgDesc> arg_descs;
if (args.ExtractArgsData(args_data, arg_descs) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}

// Each of the two parameters occupies 8 bytes, and the descriptions and data correspond in the same order.
if ((args_data.size() != 2U * sizeof(uint64_t)) ||
    (arg_descs.size() != 2U)) {
  return ge::GRAPH_FAILED;
}
return ctx.AddLaunch(launch_info, std::move(args));
```
