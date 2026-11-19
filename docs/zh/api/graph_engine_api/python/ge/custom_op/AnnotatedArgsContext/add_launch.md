# add\_launch

## 产品支持情况

全量芯片支持。

## 功能说明

提交一个kernel launch的元数据和参数声明。调用后，GE记录该launch的参数顺序，用于静态模型的地址刷新。

本方法**恒返回**本次launch的token，供后续launch声明前驱依赖。

- **省略`predecessors`、传入`None`、传入`[]`三者完全等价**：都表示本次launch无前驱依赖，且都返回有效token，因此该launch**同样可被后续launch依赖**。
- **传入非空`predecessors`**：声明本次launch的前驱依赖，同样返回本次launch的token（从0开始按提交顺序递增）。当依赖边跨越不同的逻辑流时，GE自动生成`EVENT_RECORD`/`EVENT_WAIT`任务完成跨流同步；依赖边落在同一流时不生成Event任务。

## 函数原型

```python
add_launch(
    launch_info: AnnotatedKernelLaunchInfo,
    args: AnnotatedKernelArgs,
    predecessors: Optional[Sequence[int]] = None,
) -> int
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :--- | :--- |
| launch_info | 输入 | kernel launch元数据，类型为`AnnotatedKernelLaunchInfo`，包含kernel名称、二进制、block数和stream标识。`stream_id`必须是当前上下文的主流标识（[get_stream_id](get_stream_id.md)）或由本上下文的[request_attached_stream](request_attached_stream.md)返回的逻辑辅流ID。 |
| args | 输入 | kernel参数构造器，类型为`AnnotatedKernelArgs`，必须由当前`AnnotatedArgsContext`通过[create_kernel_args](create_kernel_args.md)创建。 |
| predecessors | 输入 | 可选。前驱launch的token列表，类型为`Sequence[int]`。空列表表示无前驱但仍返回本次launch的token；元素必须是当前回调中此前成功返回的token。列表内容会被复制，不保存调用方容器。 |

## 返回值说明

| 类型 | 说明 |
| :--- | :--- |
| int | **恒返回**本次launch的token，从0开始按提交顺序递增。token仅在当前回调和当前`AnnotatedArgsContext`内有效，只能用于同一上下文后续调用的`predecessors`。 |

## 约束说明

- 此方法只能在当前`declare_launch_args`回调内调用。
- `args`必须由当前`AnnotatedArgsContext`通过`create_kernel_args`创建。`launch_info.stream_id`校验通过后才会消费`args`；此后即使底层`AddLaunch`失败也不得复用。
- `launch_info.stream_id`取值不合法时抛出`ValueError`。
- 当前`AnnotatedArgsContext`、`args`已过期时抛出`RuntimeError`。
- 至少要有一个位于主流的任务。通过`predecessors`声明流间任务的依赖路径，确保辅流任务在本算子结束前汇合到主流。GE根据已声明的跨流依赖生成Event任务，不会自动补齐缺失的汇合依赖。

## 调用示例

不声明依赖：

```python
from ge.custom_op import AnnotatedKernelLaunchInfo, get_declare_launch_args_ctx
from ge.runtime import Tensor


def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    args = ctx.create_kernel_args()
    args.append_input(0, x1)
    args.append_input(1, x2)
    args.append_output(0, y)
    kernel_bin = b"..."
    launch_info = AnnotatedKernelLaunchInfo(
        kernel_name="add_custom",
        kernel_bin=kernel_bin,
        block_dim=8,
        stream_id=ctx.get_stream_id(),
    )
    ctx.add_launch(launch_info, args)
```

声明依赖并使用辅流，跨流依赖由GE自动生成Event任务：

```python
def declare_launch_args(self, x1: Tensor, x2: Tensor, y: Tensor) -> None:
    ctx = get_declare_launch_args_ctx()
    aux_stream_id = ctx.request_attached_stream("aux")
    kernel_bin = b"..."

    # 主流launch，作为后续两个launch的前驱
    args0 = ctx.create_kernel_args()
    args0.append_input(0, x1)
    token0 = ctx.add_launch(
        AnnotatedKernelLaunchInfo(
            kernel_name="main_kernel",
            kernel_bin=kernel_bin,
            block_dim=8,
            stream_id=ctx.get_stream_id(),
        ),
        args0,
        [],
    )

    # 辅流launch，依赖主流launch
    args1 = ctx.create_kernel_args()
    args1.append_input(1, x2)
    token1 = ctx.add_launch(
        AnnotatedKernelLaunchInfo(
            kernel_name="aux_kernel",
            kernel_bin=kernel_bin,
            block_dim=8,
            stream_id=aux_stream_id,
        ),
        args1,
        [token0],
    )

    # 回到主流，依赖前两个launch，形成汇合
    args2 = ctx.create_kernel_args()
    args2.append_output(0, y)
    ctx.add_launch(
        AnnotatedKernelLaunchInfo(
            kernel_name="post_kernel",
            kernel_bin=kernel_bin,
            block_dim=8,
            stream_id=ctx.get_stream_id(),
        ),
        args2,
        [token0, token1],
    )
```
