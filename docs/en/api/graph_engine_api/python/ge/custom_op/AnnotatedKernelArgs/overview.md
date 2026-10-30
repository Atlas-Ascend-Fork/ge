# Overview

<!-- md-trans-meta sourceCommit=01f32cc4cf96f268094775f54d5f19e48a4caeea translatedAt=2026-10-15T02:30:50.413Z pushedAt=2026-10-16T10:53:07.301Z -->

`AnnotatedKernelArgs` declares the parameter order of a single kernel launch. It can only be created by [AnnotatedArgsContext.create_kernel_args](../AnnotatedArgsContext/create_kernel_args.md) and cannot be constructed directly.

Each call to `append_input`, `append_output`, `append_workspace`, or `append_scalar` appends a parameter at the end. The call order is the parameter order of the kernel launch. There is no fixed call order among these four types of methods. After appending is complete, pass the object to [AnnotatedArgsContext.add_launch](../AnnotatedArgsContext/add_launch.md) to submit it.

This object is a borrowed object and can only be used within the current `declare_launch_args` callback. The object becomes invalid after the callback ends. After `add_launch` is called, the object is consumed. Both cases cause subsequent access to raise `RuntimeError`.
