# Overview

<!-- md-trans-meta sourceCommit=01f32cc4cf96f268094775f54d5f19e48a4caeea translatedAt=2026-10-15T02:28:41.808Z pushedAt=2026-10-16T10:53:07.291Z -->

`AnnotatedArgsContext` is the declarative argument context of the current `declare_launch_args` callback, returned by [get_declare_launch_args_ctx](../get_declare_launch_args_ctx.md). Users cannot directly construct this object.

This object is provided by the native layer and is a borrowed view that is valid only within the current `declare_launch_args` callback. After the callback returns or throws an exception, this object becomes invalid; the `WorkspaceAddr` it applies for and the `AnnotatedKernelArgs` it creates also become invalid, and accessing them again throws a `RuntimeError`.
