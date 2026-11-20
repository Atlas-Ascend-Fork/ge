# serialize

<!-- md-trans-meta sourceCommit=184dd43cc1ac401fd5d103df2d9c0e00e33ecc86 translatedAt=2026-11-17T02:40:36.684Z pushedAt=2026-11-18T02:41:08.870Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Data serialization callback API for Python custom operators. Declaring the `serialize` and `deserialize` ordinary instance methods as a pair in the implementation class itself declares the offline data serialization and deserialization capability for the operator, which is referred to as the `portable` capability.

When saving an offline model, GE calls `serialize` and writes the returned `bytes` into the offline model. When loading the OM, the snapshot is restored in place into a newly created instance through [deserialize](deserialize.md).

## Prototype

```python
def serialize(self) -> bytes
```

## Parameters

None

## Return Value

| Type | Description |
| :--- | :--- |
| bytes | Returns the data serialization result of the current instance. The type must be `bytes`, and the length must be in the range [1, 4294967295], that is, it must not be empty. |

## Constraints

- `serialize` and `deserialize` must be declared as a pair in the implementation class itself. Methods with the same name inherited from the base class are not recognized. If only one of them is declared in the class itself, a `TypeError` is thrown during the registration phase.
- `serialize` may be called repeatedly, and it must return the serialization result each time.
- The serialized data should be self-contained and avoid depending on environment-related content such as the absolute path of the compilation host or device addresses.

## Examples

In the following example, `AddCustom` compiles the kernel binary during the `compile` phase and stores it in the instance state. `serialize` passes it to GE to write into the OM. When the deployment host loads the OM, it is restored by `deserialize`, and `execute` then uses it to dispatch the compute task to the RTS.

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
    """Load kernel_bin to obtain the kernel function, assemble the parameters, and dispatch the compute task to the RTS."""


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
        # The deployment host does not have the Ascend C toolchain. Use the kernel bin restored by deserialize to dispatch the task here.
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
