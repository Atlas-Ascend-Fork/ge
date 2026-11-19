# \_\_init\_\_

<!-- md-trans-meta sourceCommit=6589d3ceddaf9347af65be52af6e83b97d48d142 translatedAt=2026-10-15T02:31:25.007Z pushedAt=2026-10-16T10:53:07.306Z -->

## Applicable Products

Supported on all chips.

## Description

Creates a metadata object for a kernel launch, used to submit the kernel name, binary, block count, and stream identifier to `AnnotatedArgsContext.add_launch`.

## Prototype

```python
__init__(*, kernel_name: str, kernel_bin: bytes, block_dim: int, stream_id: int) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| :--- | :--- | :--- |
| **kernel_name** | Input | Kernel name. It cannot be an empty string. |
| **kernel_bin** | Input | Kernel binary. It cannot be an empty byte string. |
| **block_dim** | Input | Number of blocks of the kernel. The value ranges from 1 to 2^32-1. |
| **stream_id** | Input | ID of the stream where the kernel resides. The value ranges from 0 to 2^32-1. |

## Return Value

None

## Examples

`kernel_bin` represents the prepared kernel binary byte string in the example.

```python
from ge.custom_op import AnnotatedKernelLaunchInfo, get_declare_launch_args_ctx


def declare_launch_args(self, x1, x2, y) -> None:
    ctx = get_declare_launch_args_ctx()
    kernel_bin = b"..."
    launch_info = AnnotatedKernelLaunchInfo(
        kernel_name="add_custom",
        kernel_bin=kernel_bin,
        block_dim=8,
        stream_id=ctx.get_stream_id(),
    )
```

## Constraints

- Parameters must be passed by keyword.
- During construction, `kernel_name` and `kernel_bin` are copied, and this object independently holds these two pieces of data.
- If `kernel_name` is empty, `kernel_bin` is empty, or `block_dim` is 0, a `ValueError` is raised.
- When submitting, `stream_id` must match the stream identifier of the current `AnnotatedArgsContext`; if they do not match, `AnnotatedArgsContext.add_launch` raises a `ValueError`.
