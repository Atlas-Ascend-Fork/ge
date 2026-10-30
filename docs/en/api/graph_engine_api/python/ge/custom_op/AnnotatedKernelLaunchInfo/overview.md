# Overview

<!-- md-trans-meta sourceCommit=01f32cc4cf96f268094775f54d5f19e48a4caeea translatedAt=2026-10-15T02:32:05.284Z pushedAt=2026-10-16T10:53:07.304Z -->

`AnnotatedKernelLaunchInfo` describes the name, binary, number of blocks, and stream ID of a kernel launch. When calling [AnnotatedArgsContext.add_launch](../AnnotatedArgsContext/add_launch.md), submit this object together with `AnnotatedKernelArgs`.

This object is created by Python. During construction, the kernel name and binary are copied, and the object independently holds these two pieces of data. After creation, it can be saved outside the current `declare_launch_args` callback, but the `stream_id` at submission time must be the same as the stream ID of the current `AnnotatedArgsContext`.
