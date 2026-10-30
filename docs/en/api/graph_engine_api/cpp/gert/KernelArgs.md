# KernelArgs

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-28T07:53:07.297Z pushedAt=2026-10-28T08:50:45.406Z nl=2 -->

A structure that describes kernel launch parameters, including the parameter data pointer, size, and memory location information. It is mainly used for args memory management and address refresh scenarios of custom operators.

The header file is located at include/exe\_graph/runtime/kernel\_args.h under the CANN software installation path.

```c++
struct KernelArgs {
  void *args_data;      ///< Pointer to argument data
  size_t args_size;     ///< Size of argument data in bytes
  uint64_t reserved[4]; ///< Reserved for future extension
  Placement placement;  ///< Memory placement (host or device)
  uint8_t placement_reserved_[4]; ///< Reserved field, 4-byte aligned for Placement
  KernelArgs()
      : args_data(nullptr), args_size(0U), reserved{},
        placement(ge::kPlacementHost), placement_reserved_{} {}
};
```

- args\_data: Pointer to parameter data, pointing to the parameter buffer on the host or device.
- args\_size: Size of the parameter data (in bytes).
- reserved\[4\]: Reserved field for future extension.
- placement: Memory location identifier, of enumeration type.
  - ge::kPlacementHost: Host memory (host side).
  - ge::kPlacementDevice: Device memory (device side).

- placement\_reserved\_\[4\]: Reserved field, 4-byte aligned.
