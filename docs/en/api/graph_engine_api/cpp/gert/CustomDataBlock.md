# CustomDataBlock

<!-- md-trans-meta sourceCommit=cdbc38c7db7112e5c8cb9d0214451a84c91c89f6 translatedAt=2026-11-17T02:45:22.909Z pushedAt=2026-11-18T02:41:08.887Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

`gert::CustomDataBlock` is a variable-length custom data descriptor. As a parameter of [`AnnotatedKernelArgs::AppendArg`](AnnotatedKernelArgs/AppendArg.md), it is used to append a segment of read-only byte data of unfixed length to kernel args. Typical scenarios include tiling data, constant lookup tables, and descriptor arrays.

The data is delivered with the model and uploaded to the device once during model loading. It occupies only one 8-byte slot in kernel args, and the slot value is the device address of the data. The kernel side needs to read the actual custom data from the device address.

The difference between `CustomDataBlock` and `CustomValue` (that is, `AppendArg(uint64_t)`) is as follows: `CustomValue` inlines the 8-byte value directly in the kernel args slot, and the kernel receives it by value without dereferencing. The slot of `CustomDataBlock` is the device address of the data, and the kernel needs to dereference and read it by address. The data is read-only, and its length is not delivered along with the kernel args.

## Prototype

```c++
namespace gert {
struct CustomDataBlock {
  const void *data;    // Data start address (host). Deep copy when appending. The caller memory can be modified or released immediately after return.
  size_t size;         // Data length in bytes. Must be greater than 0.
  // Binary-compatible reserved field. When adding fields, reduce the length of this array to maintain sizeof(CustomDataBlock) unchanged.
  // In the current version, must all be set to 0.
  uint8_t reserved[8] = {0U};
};
}  // namespace gert
```

## Parameters

| Name | Type | Description |
| --- | --- | --- |
| `data` | `const void *` | Data start address, pointing to host-side memory. This memory is deep-copied before `AppendArg` returns, so the caller can modify or release it immediately after return. It must not be null when `size` is greater than 0. |
| `size` | `size_t` | Data length in bytes. Must be greater than 0. This length is used only for host-side memory allocation and data upload. It is not written to kernel args and is not delivered to the kernel along with kernel args. |
| `reserved` | `uint8_t[8]` | Binary-compatible reserved field. In the current version, must all be set to 0, and the framework does not use it for now. When it is non-zero, `AppendArg` returns `GRAPH_FAILED`. When reserved fields are enabled in subsequent versions, newly added fields will reduce the length of this array to maintain `sizeof(CustomDataBlock)` unchanged. |

## Return Value

None

## Constraints

- The bytes pointed to by `data` are deep-copied when appending and fixed at compile time, and are read-only at execution time. For scenarios that require rewriting content at execution time, use [`AnnotatedArgsContext::MallocWorkSpace`](AnnotatedArgsContext/MallocWorkSpace.md).
- `size` is not delivered with the kernel args. The kernel side needs to determine the length based on the data form (see "Examples" for the three methods):
  - Fixed-length data: The host side and the kernel side use the same structure definition, and the kernel side restores it by `sizeof(structure)`.
  - Variable-length data (recommended): The first field or header structure carries magic and the total length (or entry count). The kernel side validates first and then reads.
  - Length slot: After `AppendArg(const CustomDataBlock &)`, call `AppendArg(uint64_t{size})` again. The kernel side receives one additional length parameter, occupying one more 8-byte slot.
- The host side and the kernel side must use the same data layout definition (the same header file and the same compile options). Otherwise, the length and field offsets will be inconsistent.
- In the current version, the device address of each data block (blob) is aligned to 512 bytes. The padding bytes between blobs and at the end are 0. The framework does not perform out-of-bounds access validation. The kernel side must limit the access range based on the length field. Reading past the end of a blob returns padding 0, but reading past the padding area of the last blob is illegal memory access (it may read other data or trigger an exception).
- The data volume of a single blob and the cumulative data volume of a single launch must not exceed `UINT32_MAX`. The number of args slots in a single launch must not exceed 8192.
- Variable-length custom data is not supported in the following scenarios, and an explicit error is reported at compile time:
  - End-side offline model (.omc).
  - OM2 model.
- `reserved` must all be set to 0. This field is reserved for extension in subsequent versions. When it is non-zero, `AppendArg` returns `GRAPH_FAILED`, preventing historical dirty values from being misinterpreted as valid semantics in subsequent versions.

## Examples

`size` is not delivered with the kernel args. Select one of the following methods based on data form.

### Fixed-length Data: Shared Struct

```c++
struct MyTiling {
  uint32_t totalLength;  // Total data length, for kernel-side validation.
  uint32_t tileNum;
};

const auto *input = ctx.GetInputTensor(0U);
const auto *output = ctx.GetOutputTensor(0U);
if ((input == nullptr) || (output == nullptr)) {
  return ge::GRAPH_FAILED;
}

MyTiling tiling{static_cast<uint32_t>(sizeof(MyTiling)), 8U};
gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::CustomDataBlock{&tiling, sizeof(tiling)}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::OutputAddr{0U, output->GetAddr()}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
// The args occupy three 8-byte slots in the above order, where the value of the second slot is the device address of the tiling data.
return ctx.AddLaunch(launch_info, std::move(args));
```

The corresponding kernel entry function has three parameters, and the second parameter receives the device address of this data:

```c++
extern "C" __global__ __aicore__ void my_kernel(GM_ADDR x, GM_ADDR tiling, GM_ADDR y) {
  // The length is not passed through args. Restore the data using the structure definition shared by the host and device, and validate the length before reading.
  __gm__ const MyTiling *const tilingData = reinterpret_cast<__gm__ const MyTiling *>(tiling);
  if (tilingData->totalLength != static_cast<uint32_t>(sizeof(MyTiling))) {
    return;
  }
  // After validation passes, access the remaining data by tilingData->tileNum.
}
```

### Variable-length Data: Self-describing Header

Write magic and the entry count into the data header. The kernel side validates the header first and then locates the variable-length area by the length carried by the data itself:

```c++
constexpr uint32_t kTableMagic = 0x5A5A0000U;
constexpr uint32_t kMaxTableLen = 16U;  // Upper limit of the entry count. The host side must ensure it is not exceeded during construction.
struct MyTableHeader {
  uint32_t magic;     // For kernel-side validation of the data layout.
  uint32_t tableLen;  // Entry count of the variable-length area.
};

const auto *input = ctx.GetInputTensor(0U);
if (input == nullptr) {
  return ge::GRAPH_FAILED;
}

const std::vector<int32_t> table = {1, 2, 3, 4, 5, 6};  // The entry count is determined by the business and fixed at compile time.
std::vector<uint8_t> blob(sizeof(MyTableHeader) + table.size() * sizeof(int32_t), 0U);
auto *const header = reinterpret_cast<MyTableHeader *>(blob.data());
header->magic = kTableMagic;
header->tableLen = static_cast<uint32_t>(table.size());
std::copy(table.begin(), table.end(), reinterpret_cast<int32_t *>(header + 1));

gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::CustomDataBlock{blob.data(), blob.size()}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
return ctx.AddLaunch(launch_info, std::move(args));
```

The second parameter of the corresponding kernel entry function receives the device address of this data, and the position of the variable-length area is derived from the header information:

```c++
extern "C" __global__ __aicore__ void my_kernel(GM_ADDR x, GM_ADDR table) {
  // Validate the header first (kTableMagic/kMaxTableLen come from the header file shared by the host and the kernel), then access the variable-length area by the entry count carried by the blob.
  __gm__ const MyTableHeader *const header = reinterpret_cast<__gm__ const MyTableHeader *>(table);
  if ((header->magic != kTableMagic) || (header->tableLen > kMaxTableLen)) {
    return;
  }
  __gm__ const int32_t *const values = reinterpret_cast<__gm__ const int32_t *>(header + 1);
  // Access the variable-length area by values[0 .. header->tableLen).
}
```

### Length Slot: Appending a uint64_t Parameter

When the data itself does not carry a length field, append a `uint64_t` length slot after `CustomDataBlock` (occupying one additional 8-byte slot). The kernel side receives one additional length parameter:

```c++
struct MyParams {  // Structure definition shared on the host side and the device side.
  uint32_t tileNum;
  uint32_t blockNum;
};

const auto *input = ctx.GetInputTensor(0U);
const auto *output = ctx.GetOutputTensor(0U);
if ((input == nullptr) || (output == nullptr)) {
  return ge::GRAPH_FAILED;
}

MyParams params{8U, 8U};
gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::CustomDataBlock{&params, sizeof(params)}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(uint64_t{sizeof(params)}) != ge::GRAPH_SUCCESS ||  // Length slot.
    args.AppendArg(gert::OutputAddr{0U, output->GetAddr()}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
// The args have four slots in total, in order: input address, params address, params length, and output address.
return ctx.AddLaunch(launch_info, std::move(args));
```

```c++
extern "C" __global__ __aicore__ void my_kernel(GM_ADDR x, GM_ADDR params, uint64_t paramsLen, GM_ADDR y) {
  // The length is passed in through an independent slot, so the data itself does not need to carry a length field.
  __gm__ const MyParams *const paramsData = reinterpret_cast<__gm__ const MyParams *>(params);
  if (paramsLen != sizeof(MyParams)) {
    return;
  }
  // Access by paramsData->tileNum and paramsData->blockNum.
}
```
