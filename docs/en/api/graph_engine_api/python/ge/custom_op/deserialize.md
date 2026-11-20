# deserialize

<!-- md-trans-meta sourceCommit=184dd43cc1ac401fd5d103df2d9c0e00e33ecc86 translatedAt=2026-11-17T02:40:09.798Z pushedAt=2026-11-18T02:41:08.866Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Callback API for deserializing Python custom operator data in an offline model. It is declared in pairs with [serialize](serialize.md) in the implementation class itself, which declares the offline-scenario data serialization and deserialization capability for the operator, referred to as the `portable` capability.

## Prototype

```python
def deserialize(self, data: bytes) -> None
```

## Parameters

| Name | Input/Output | Description |
| :--- | :--- | :--- |
| data | Input | Data snapshot returned by `serialize` when the offline model is saved. The type is `bytes`, with a length range of `[1, 4294967295]`. This object has independent storage and does not reference the GE internal buffer, so it can still be held after the method returns. The parameter name is not required. |

## Constraints

- `serialize` and `deserialize` must be declared in pairs in the implementation class itself. Methods with the same name inherited from the base class are not recognized. If only one of them is declared in the class itself, a `TypeError` is raised during the registration phase.
- Data restoration must modify `self` in place and cannot be passed through a return value.

## Examples

In the following example, when `AddCustom` loads the OM, `deserialize` restores the kernel binary compiled during the `compile` phase, and the subsequent `execute` directly uses it to deliver computing tasks to the RTS. The deployment host does not need to have the Ascend C toolchain.

```python
import base64
import json

from ge.custom_op import (
    get_compile_platform_info,
    get_execute_ctx,
    register_op_impl,
)
from ge.runtime import Tensor

_BLOCK_SIZE = 1024


def compile_kernel(platform) -> bytes:
    """Call the Ascend C toolchain to compile the kernel source code and return the device binary."""


def launch_kernel(kernel_bin, block_dim, x_addr, y_addr, z_addr, stream) -> None:
    """Load kernel_bin to obtain the kernel function, assemble the parameters, and deliver the computing task to the RTS."""


@register_op_impl(op_type="AddCustom")
class AddCustom:
    def __init__(self) -> None:
        self._kernel_bin = b""
        self._block_dim = 0

    def compile(self, x: Tensor, y: Tensor, z: Tensor) -> None:
        if self._kernel_bin:
            return
        self._kernel_bin = compile_kernel(get_compile_platform_info())
        self._block_dim = x.shape_size // _BLOCK_SIZE

    def execute(self, x: Tensor, y: Tensor) -> None:
        ctx = get_execute_ctx()
        z = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
        # The deployment host does not have the Ascend C toolchain. Use the kernel bin restored by deserialize to deliver the task here.
        launch_kernel(
            self._kernel_bin, self._block_dim, x.addr, y.addr, z.addr, ctx.get_stream()
        )

    def serialize(self) -> bytes:
        state = {
            "block_dim": self._block_dim,
            "kernel_bin": base64.b64encode(self._kernel_bin).decode("ascii"),
        }
        return json.dumps(state).encode("utf-8")

    def deserialize(self, data: bytes) -> None:
        state = json.loads(data.decode("utf-8"))
        self._block_dim = state["block_dim"]
        self._kernel_bin = base64.b64decode(state["kernel_bin"])
```
