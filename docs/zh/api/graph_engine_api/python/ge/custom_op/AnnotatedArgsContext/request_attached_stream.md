# request\_attached\_stream

## 产品支持情况

全量芯片支持。

## 功能说明

按key申请可复用的逻辑辅流。相同key返回同一逻辑stream ID，可作为[AnnotatedKernelLaunchInfo](../AnnotatedKernelLaunchInfo/AnnotatedKernelLaunchInfo.md)的`stream_id`在辅流上提交kernel launch；不同key返回不同的逻辑stream ID。

key的作用域是算子所在的**编译子图**（对应一个编译子模型），同一子图内相同key复用同一辅流，**不跨子图复用**。不同子图即使key相同也会得到各自的辅流。

申请到的辅流由GE在模型加载期按扩展后的流数量创建为物理流，并作为模型的普通逻辑流参与下沉与重放。跨流的launch依赖由GE根据[add_launch](add_launch.md)的`predecessors`自动生成`EVENT_RECORD`/`EVENT_WAIT`任务，算子无需自行同步。

## 函数原型

```python
request_attached_stream(key: str) -> int
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :--- | :--- |
| key | 输入 | 辅流标识，类型为`str`，不能为空字符串。作用域为算子所在的编译子图。 |

## 返回值说明

| 类型 | 说明 |
| :--- | :--- |
| int | key对应的逻辑辅流ID。仅是编译期逻辑ID，不是运行时stream句柄。 |

## 约束说明

- 只能在当前`declare_launch_args`回调内调用。
- `key`为空字符串时抛出`ValueError`。辅流申请失败或当前`AnnotatedArgsContext`已失效时抛出`RuntimeError`。
- 返回的逻辑辅流ID不得缓存到回调外，只能用于**当前**`AnnotatedArgsContext`构造`AnnotatedKernelLaunchInfo`并传给同一上下文的[add_launch](add_launch.md)。传入未经本上下文申请的stream ID会在消费`AnnotatedKernelArgs`之前抛出`ValueError`。
- 至少要有一个位于主流（`get_stream_id()`返回值）的任务。
- `CANN-FMK-`是框架内部使用的辅流key前缀（如HCCL），用户自定义算子应避免以该前缀命名key。

## 调用示例

```python
from ge.custom_op import AnnotatedKernelLaunchInfo, get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    aux_stream_id = ctx.request_attached_stream("aux")
    kernel_bin = self._kernel_bin

    # 主流launch
    main_info = AnnotatedKernelLaunchInfo(
        kernel_name="main_kernel",
        kernel_bin=kernel_bin,
        block_dim=8,
        stream_id=ctx.get_stream_id(),
    )
    main_args = ctx.create_kernel_args()
    main_args.append_input(0, x1)
    token_main = ctx.add_launch(main_info, main_args)

    # 辅流launch，依赖主流launch，GE自动为跨流依赖生成Event任务
    aux_info = AnnotatedKernelLaunchInfo(
        kernel_name="aux_kernel",
        kernel_bin=kernel_bin,
        block_dim=8,
        stream_id=aux_stream_id,
    )
    aux_args = ctx.create_kernel_args()
    aux_args.append_input(1, x2)
    token_aux = ctx.add_launch(aux_info, aux_args, [token_main])

    # 回到主流，依赖辅流launch，形成汇合
    post_info = AnnotatedKernelLaunchInfo(
        kernel_name="post_kernel",
        kernel_bin=kernel_bin,
        block_dim=8,
        stream_id=ctx.get_stream_id(),
    )
    post_args = ctx.create_kernel_args()
    post_args.append_output(0, y)
    ctx.add_launch(post_info, post_args, [token_main, token_aux])
```
