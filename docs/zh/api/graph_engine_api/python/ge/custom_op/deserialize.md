# deserialize

## 产品支持情况

全量芯片支持。

## 功能说明

恢复离线模型中Python自定义算子数据的反序列化回调接口。与[serialize](serialize.md)成对声明在实现类自身，即为该算子声明离线场景数据序列化与反序列化能力，称之为`portable`能力。

## 函数原型

```python
def deserialize(self, data: bytes) -> None
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :--- | :--- |
| data | 输入 | 保存离线模型时`serialize`返回的数据快照。类型为`bytes`，长度范围为`[1, 4294967295]`。该对象拥有独立存储，不引用GE内部缓冲区，方法返回后仍可持有。参数名不作要求。 |

## 约束说明

- `serialize`和`deserialize`必须成对声明在实现类自身，从基类继承的同名方法不参与识别；类自身只声明其中一个时，注册阶段抛出`TypeError`。
- 数据恢复必须就地修改`self`，不能通过返回值传递。

## 调用示例

以下示例中，`AddCustom`加载OM时由`deserialize`恢复`compile`阶段编译出的kernel二进制，随后的`execute`直接用它向RTS下发计算任务，部署主机无需具备Ascend C工具链。

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
    """调用Ascend C工具链编译kernel源码，返回device二进制。"""


def launch_kernel(kernel_bin, block_dim, x_addr, y_addr, z_addr, stream) -> None:
    """加载kernel_bin取出核函数，组装参数后向RTS下发计算任务。"""


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
        # 部署主机不具备Ascend C工具链，这里使用deserialize恢复出的kernel bin下发任务。
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
