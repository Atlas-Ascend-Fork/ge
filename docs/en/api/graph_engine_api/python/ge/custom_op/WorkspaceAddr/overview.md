# Overview

<!-- md-trans-meta sourceCommit=01f32cc4cf96f268094775f54d5f19e48a4caeea translatedAt=2026-10-15T02:40:05.869Z pushedAt=2026-10-16T10:53:07.342Z -->

`WorkspaceAddr` describes the workspace allocated by [AnnotatedArgsContext.malloc_workspace](../AnnotatedArgsContext/malloc_workspace.md). This object can be passed to `AnnotatedKernelArgs.append_workspace` to declare the workspace address parameter of a kernel.

`WorkspaceAddr` cannot be constructed directly and can only be returned by `malloc_workspace()`. It is a borrowed object that is valid only within the current `declare_launch_args` callback. After the callback returns or throws an exception, it becomes invalid together with `AnnotatedArgsContext`, and accessing it again raises `RuntimeError`.
