# Overview

<!-- md-trans-meta sourceCommit=cdbc38c7db7112e5c8cb9d0214451a84c91c89f6 translatedAt=2026-11-17T02:41:32.033Z pushedAt=2026-11-18T02:41:08.872Z nl=2 -->

`gert::AnnotatedKernelArgs` is a declarative kernel launch parameter builder. The caller appends logical input addresses, logical output addresses, logical workspace addresses, variable-length custom data, and `uint64_t` scalars in sequence according to the kernel parameter layout, and then passes the object as an rvalue to [`gert::AnnotatedArgsContext::AddLaunch`](../AnnotatedArgsContext/AddLaunch.md). This API is used only in the compile-time callback of [`ge::AnnotatedArgsOp::DeclareLaunchArgs`](../../ge/AnnotatedArgsOp/DeclareLaunchArgs.md) to construct the launch parameters of a static graph.

## Header File to Include

```c++
#include <exe_graph/runtime/annotated_args_context.h>
```

## Public Member Functions

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
ge::graphStatus AppendArg(const InputAddr &addr)
ge::graphStatus AppendArg(const OutputAddr &addr)
ge::graphStatus AppendArg(const WorkspaceAddr &addr)
ge::graphStatus AppendArg(const CustomDataBlock &data)
ge::graphStatus AppendArg(uint64_t value)
ge::graphStatus ExtractArgsData(std::vector<uint8_t> &args_data,
                                std::vector<ge::ArgDesc> &arg_descs) const
```

## Related Types

- [`InputAddr`](../InputAddr.md)
- [`OutputAddr`](../OutputAddr.md)
- [`WorkspaceAddr`](../WorkspaceAddr.md)
- [`CustomDataBlock`](../CustomDataBlock.md)
- [`AnnotatedArgsContext`](../AnnotatedArgsContext/overview.md)
- [`AnnotatedKernelLaunchInfo`](../AnnotatedKernelLaunchInfo.md)

For copy, move, and destruction behavior, see [Constructors, Assignment Operators, and Destructor](AnnotatedKernelArgs_constructor_and_destructor.md).
