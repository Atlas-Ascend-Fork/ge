# AnnotatedKernelArgs Constructor, Assignment Operator, and Destructor

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T07:59:57.661Z pushedAt=2026-10-28T08:50:45.436Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <exe\_graph/runtime/annotated\_args\_context.h\>
- Library file: liblowering.so

## Description

Constructs, copies, moves, or destroys a `gert::AnnotatedKernelArgs` object. The object internally owns the storage for the argument byte data and argument descriptions; copy operations duplicate this data, while move operations transfer ownership.

## Prototype

```c++
AnnotatedKernelArgs()
template <typename Arg, typename... Args,
          typename = typename std::enable_if<
              !std::is_same<typename std::decay<Arg>::type, AnnotatedKernelArgs>::value>::type>
explicit AnnotatedKernelArgs(Arg &&arg, Args &&...args)
AnnotatedKernelArgs(const AnnotatedKernelArgs &other)
AnnotatedKernelArgs(AnnotatedKernelArgs &&other) noexcept
AnnotatedKernelArgs &operator=(const AnnotatedKernelArgs &other)
AnnotatedKernelArgs &operator=(AnnotatedKernelArgs &&other) noexcept
~AnnotatedKernelArgs()
```

## Parameters

| Name | Input/Output | Description |
| --- | --- | --- |
| `arg` | Input | First argument of the variadic constructor. Its type must be `InputAddr`, `OutputAddr`, `WorkspaceAddr`, or `uint64_t`. |
| `args` | Input | Remaining arguments of the variadic constructor, appended in the order passed. |
| `other` | Input | Source object to be copied or moved. |

## Return Value

- The constructor returns the constructed `AnnotatedKernelArgs` object.
- The copy assignment and move assignment return `*this`.
- The destructor returns no value.

## Constraints

The following constraints specify both the object lifetime and the move semantics.

- The default constructor creates an empty builder. An empty builder must first successfully append at least one argument before it can call [`ExtractArgsData`](ExtractArgsData.md) or be submitted to [`AnnotatedArgsContext::AddLaunch`](../AnnotatedArgsContext/AddLaunch.md).
- The variadic constructor calls `AppendArg` in the order of the arguments. If an append operation fails, the remaining arguments are not appended. The object in this state cannot be used as valid launch parameters and should be checked and discarded.
- Copy construction and copy assignment perform a deep copy: the source and destination own independent argument bytes and description arrays, so subsequent appends to either object do not affect the other. The address field copies the address value itself, not the memory the address points to.
- Move construction and move assignment transfer the internal storage; after the move completes, the source object must not be used again. Calling any `AppendArg` on a moved-from source object, or extracting arguments from it, must not be relied upon to succeed.
- Self-move assignment (`obj = std::move(obj)`) leaves the object usable; except for this special case, do not continue to access the source object after a move.
- After passing the object to `AddLaunch` as an rvalue, GE extracts and copies the data required for the launch from the object; the caller must not rely on the state of the object after the call, nor continue to append or extract arguments.

## Examples

```c++
gert::AnnotatedKernelArgs original(
    gert::InputAddr{0U, input_addr}, uint64_t{1U});

// After the copy, the two objects are independent of each other.
gert::AnnotatedKernelArgs copied(original);
(void)copied.AppendArg(gert::OutputAddr{0U, output_addr});

// moved acquires the storage of copied; copied must not be used after the move.
gert::AnnotatedKernelArgs moved(std::move(copied));
const auto ret = ctx.AddLaunch(launch_info, std::move(moved));
if (ret != ge::GRAPH_SUCCESS) {
  return ret;
}
```

The `input_addr`, `output_addr`, and `launch_info` in the example should be obtained from the current [`DeclareLaunchArgs`](../../ge/AnnotatedArgsOp/DeclareLaunchArgs.md) callback; for detailed validation of `AddLaunch`, see [`AnnotatedKernelLaunchInfo`](../AnnotatedKernelLaunchInfo.md). `AddLaunch` does not immediately invalidate the internal storage of `AnnotatedKernelArgs` because of the rvalue reference, but the caller should still not rely on the state of the object after the call.
