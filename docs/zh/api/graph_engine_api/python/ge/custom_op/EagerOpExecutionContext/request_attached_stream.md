# request\_attached\_stream

## 产品支持情况

全量芯片支持。

## 功能说明

按key申请框架托管的物理辅流，供Eager类Python自定义算子在[execute](../execute.md)回调中把kernel下发到辅流上执行，与执行流并发。

- 首次申请时框架创建物理流；同一模型执行实例内使用相同key的后续申请直接返回已创建的流句柄，支持跨节点、跨算子类型复用。
- 不同key返回不同的流句柄；不同模型执行实例之间的辅流相互隔离，即使key相同也不共享。

## 函数原型

```python
request_attached_stream(key: str, flags: int = 0) -> Optional[int]
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :--- | :--- |
| key | 输入 | 辅流复用key，类型为`str`，不能为空字符串。 |
| flags | 输入 | 流创建标志，类型为`int`，默认`0`。传入`RT_STREAM_*`宏对应的整数值，多个标志可按位或组合。取值以所安装CANN版本的`rt_external_stream.h`为准，可参考[Runtime源码中的标志定义](https://gitcode.com/cann/runtime/blob/master/pkg_inc/runtime/rt_external_stream.h#L22-L43)。仅首次创建辅流时生效，复用已有辅流时忽略。 |

## 返回值说明

| 类型 | 说明 |
| :--- | :--- |
| int | 申请成功时返回框架托管辅流句柄的整数表示，可直接作为`acl.rt.launch_kernel_with_config`等接口的stream入参。 |
| None | 当前执行链路不支持辅流申请，或辅流创建、绑定模型失败时返回`None`。调用方可降级到[get_stream](get_stream.md)返回的执行流。 |

## 约束说明

- 仅可在当前`execute`调用栈内调用。`key`为空字符串时抛出`ValueError`，上下文失效后用有效`key`调用时抛出`RuntimeError`。
- 算子需用Event将辅流任务汇合到[get_stream](get_stream.md)返回的执行流，保证辅流kernel执行完毕后框架才回收或复用参数与输出内存。模型录制场景使用`acl.rt.create_event_with_flag`创建Event，标志组合见下方示例。每次调用`acl.rt.stream_wait_event`后，在同一条流上调用`acl.rt.reset_event`，清除本轮触发状态。
- 辅流由框架管理，算子仅在当前回调中使用返回的流句柄，且不得调用`acl.rt.destroy_stream`销毁句柄。
- pyACL Event由算子管理。静态模型录制的任务会在重放时引用Event，必须等引用它的任务全部完成且模型无需再重放后才能销毁。如果同一算子实现服务多个模型执行实例，需要为各执行实例分别管理Event，避免共享示例中的一对Event。
- 此接口不支持并发调用。
- `CANN-FMK-`是框架内部使用的辅流key前缀（如HCCL），用户自定义算子应避免以该前缀命名key。

## 调用示例

以下示例使用pyACL接口加载kernel二进制、组装参数并向辅流下发kernel。示例适用于单个模型执行实例中的单个算子节点，且各轮执行不重叠。

```python
import acl
import ctypes

from ge.custom_op import get_execute_ctx, register_op_impl

_AUX_KEY = "my_aux"
_BLOCK_SIZE = 1024


def _check_acl(ret, action):
    if ret != 0:
        raise RuntimeError("{} failed, ret={}".format(action, ret))


# 取值来自acl_rt.h，依次为ACL_EVENT_SYNC、ACL_EVENT_CAPTURE_STREAM_PROGRESS、ACL_EVENT_TIME_LINE
_EVENT_FLAGS = 0x1 | 0x2 | 0x8


@register_op_impl(op_type="MyAttachedStreamOp")
class MyAttachedStreamOp:
    def __init__(self):
        # 保留本示例使用的Event，供模型重放时复用
        self._ev_in = None
        self._ev_out = None

    def _ensure_events(self):
        if self._ev_in is not None:
            return
        # 返回(event, ret)元组，必须解包
        ev_in, ret = acl.rt.create_event_with_flag(_EVENT_FLAGS)
        _check_acl(ret, "acl.rt.create_event_with_flag(ev_in)")
        ev_out, ret = acl.rt.create_event_with_flag(_EVENT_FLAGS)
        _check_acl(ret, "acl.rt.create_event_with_flag(ev_out)")
        self._ev_in, self._ev_out = ev_in, ev_out

    def execute(self, x, y) -> None:
        ctx = get_execute_ctx()
        z = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)

        main_stream = ctx.get_stream()
        aux_stream = ctx.request_attached_stream(_AUX_KEY)
        if aux_stream is None:
            raise RuntimeError("failed to request attached stream")

        self._ensure_events()
        ev_in, ev_out = self._ev_in, self._ev_out

        # 1. 在执行流上记录Event，在辅流上等待并重置，确保辅流kernel读取已就绪的输入
        _check_acl(acl.rt.record_event(ev_in, main_stream), "acl.rt.record_event")
        _check_acl(acl.rt.stream_wait_event(aux_stream, ev_in), "acl.rt.stream_wait_event")
        _check_acl(acl.rt.reset_event(ev_in, aux_stream), "acl.rt.reset_event")

        # 2. 组装参数并把kernel下发到辅流
        func_handle = self._load_kernel()
        args_handle, ret = acl.rt.kernel_args_init(func_handle)
        _check_acl(ret, "acl.rt.kernel_args_init")
        # ctypes对象必须保持有效直到launch返回，否则参数指向的宿主内存会被回收
        keep_alive = []
        for addr in (x.addr, y.addr, z.addr):
            holder = ctypes.c_uint64(int(addr))
            keep_alive.append(holder)
            _check_acl(
                acl.rt.kernel_args_append(args_handle, ctypes.addressof(holder), ctypes.sizeof(holder)),
                "acl.rt.kernel_args_append",
            )
        _check_acl(acl.rt.kernel_args_finalize(args_handle), "acl.rt.kernel_args_finalize")
        blocks = int(x.shape_size) // _BLOCK_SIZE
        _check_acl(
            acl.rt.launch_kernel_with_config(func_handle, blocks, aux_stream, [], args_handle, 0),
            "acl.rt.launch_kernel_with_config",
        )

        # 3. 在辅流上记录Event，在执行流上等待并重置，使辅流任务汇合到执行流
        _check_acl(acl.rt.record_event(ev_out, aux_stream), "acl.rt.record_event")
        _check_acl(acl.rt.stream_wait_event(main_stream, ev_out), "acl.rt.stream_wait_event")
        _check_acl(acl.rt.reset_event(ev_out, main_stream), "acl.rt.reset_event")
        # 引用这些Event的任务执行完毕且模型无需再重放后，方可销毁Event
```
