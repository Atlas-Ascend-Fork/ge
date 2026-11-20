# append\_custom\_data\_block

## 产品支持情况

全量芯片支持。

## 功能说明

向当前kernel参数序列追加只读自定义数据块，例如tiling数据或常量表。调用时深拷贝数据，返回后无需保留原字节串。

数据随模型保存，在模型加载期上传至device。参数序列中占用一个8字节指针槽，kernel通过该device地址读取数据；数据长度不会自动传给kernel。

## 函数原型

```python
append_custom_data_block(data: bytes) -> None
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :--- | :--- |
| data | 输入 | 非空bytes，允许包含零字节。可通过`bytes(bytearray_value)`或`array.tobytes()`转换其他数据。字节布局和端序必须与kernel一致。 |

## 返回值说明

无

## 约束说明

- 只能对当前`declare_launch_args`回调中由`create_kernel_args()`创建的对象调用；传入`add_launch`后不可复用。
- 可与输入、输出、workspace及标量参数混排，调用顺序决定参数顺序；允许追加多个数据块。
- 数据在编译期固化，执行期只读；运行期可写数据应使用[append_workspace](append_workspace.md)。
- kernel需通过约定的固定布局、自描述头部，或额外的`append_scalar(len(data))`长度槽确定长度。
- 单个数据块和单次launch累计数据量不得超过`UINT32_MAX`，参数槽位数不得超过8192。超出这些上限会导致编译期生成任务失败，不会在追加时检查。
- 数据为空时抛出`ValueError`，类型不是`bytes`时抛出`TypeError`。参数构造器已失效或已被消费，或者底层追加返回失败时，抛出`RuntimeError`。
- 数据块device地址按512字节对齐，kernel必须自行保证访问不越界。
- 不支持端侧离线模型和OM2模型，编译期显式报错。

## 调用示例

```python
import struct
from ge.custom_op import AnnotatedKernelLaunchInfo, get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    data = struct.pack("<II", 8, 16)
    args.append_input(0, x1)
    args.append_input(1, x2)
    args.append_output(0, y)
    args.append_custom_data_block(data)
    args.append_scalar(len(data))
    launch_info = AnnotatedKernelLaunchInfo(
        kernel_name="add_custom",
        kernel_bin=b"...",  # 替换为实际的kernel二进制字节串。
        block_dim=8,
        stream_id=ctx.get_stream_id(),
    )
    ctx.add_launch(launch_info, args)
```
