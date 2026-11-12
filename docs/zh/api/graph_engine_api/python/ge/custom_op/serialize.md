# serialize

## 产品支持情况

全量芯片支持。

## 功能说明

Python自定义算子的数据序列化回调接口。在实现类自身成对声明`serialize`和`deserialize`普通实例方法，即为该算子声明离线场景数据序列化与反序列化能力，称之为`portable`能力。

GE在保存离线模型时调用`serialize`，把返回的`bytes`写入离线模型；加载OM时通过[deserialize](deserialize.md)把该快照就地恢复进新建实例。

## 函数原型

```python
def serialize(self) -> bytes
```

## 参数说明

无

## 返回值说明

| 类型 | 说明 |
| :--- | :--- |
| bytes | 返回当前实例的数据序列化结果，类型必须是`bytes`，长度范围为`[1, 4294967295]`，即不得为空 |

## 约束说明

- `serialize`和`deserialize`必须成对声明在实现类自身，从基类继承的同名方法不参与识别；类自身只声明其中一个时，注册阶段抛出`TypeError`。
- `serialize`可能被重复调用，每次都必须返回序列化结果。
- 序列化数据应自包含，避免依赖编译主机的绝对路径、设备地址等环境相关内容。

## 调用示例

以下示例中，`AddCustom`在`compile`阶段编译出kernel二进制并存入实例状态，`serialize`把它交给GE写入OM；部署主机加载OM时由`deserialize`恢复，`execute`再用它向RTS下发计算任务。

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
