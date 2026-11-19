# Overview

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:12:32.222Z pushedAt=2026-10-16T10:53:07.699Z -->

`ge.runtime.Tensor` is a tensor runtime view used during the execution callback of a Python custom operator. It provides metadata such as the tensor address, size, Shape, Format, DataType, and storage location, and does not allocate, release, or copy tensor data.

`Tensor` is provided by the GE runtime through `execute`, `declare_launch_args`, or the execution context, and cannot be directly constructed by users. The `Tensor` returned by the callback, along with its `Shape`, `StorageShape`, `StorageFormat`, and `ExpandDimsType` views, is valid only within the current callback.

The following example reads the metadata of the input Tensor in the `execute` callback and allocates the output Tensor based on the Shape, Format, and DataType of the input Tensor.

```python
from ge.custom_op import get_execute_ctx, register_op_impl
from ge.runtime import Tensor


@register_op_impl(op_type="AddPythonCustomOp")
class AddPythonCustomOp:
    def execute(self, x: Tensor, y: Tensor) -> None:
        ctx = get_execute_ctx()
        output = ctx.malloc_output_tensor(0, x.shape, x.format, x.data_type)
        print(x.addr, x.size, x.storage_shape.dims, x.placement)
        # output is the Tensor in the current execution callback and can continue to be used for execution parameter construction.
        _ = output
```
