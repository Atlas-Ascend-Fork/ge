# Overview

<!-- md-trans-meta sourceCommit=f0a9fb067be0ab503b9c0f2dd50680654a1a1277 translatedAt=2026-10-28T08:01:14.680Z pushedAt=2026-10-28T08:50:45.439Z nl=2 -->

`gert::AnnotatedKernelArgs` is a declarative kernel launch parameter builder. The caller appends logical input addresses, logical output addresses, logical workspace addresses, and `uint64_t` scalars in sequence according to the kernel parameter layout, and then passes the object as an rvalue to [`gert::AnnotatedArgsContext::AddLaunch`](../AnnotatedArgsContext/AddLaunch.md). This API is used only in the [`ge::AnnotatedArgsOp::DeclareLaunchArgs`](../../ge/AnnotatedArgsOp/DeclareLaunchArgs.md) compile-time callback to construct the launch parameters of a static graph.

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
ge::graphStatus AppendArg(uint64_t value)
ge::graphStatus ExtractArgsData(std::vector<uint8_t> &args_data,
                                std::vector<ge::ArgDesc> &arg_descs) const
```

## Related Types

- [`InputAddr`](../InputAddr.md)
- [`OutputAddr`](../OutputAddr.md)
- [`WorkspaceAddr`](../WorkspaceAddr.md)
- [`AnnotatedArgsContext`](../AnnotatedArgsContext/overview.md)
- [`AnnotatedKernelLaunchInfo`](../AnnotatedKernelLaunchInfo.md)

For copy, move, and destruction behavior, see [Constructors, Assignment Operators, and Destructor](AnnotatedKernelArgs_constructor_and_destructor.md).
