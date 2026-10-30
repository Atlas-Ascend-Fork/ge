# Overview

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:15:33.427Z pushedAt=2026-10-16T10:53:07.711Z -->

The `TensorDesc` class represents the tensor metadata of the Python custom operator `infer_meta`, storing the logical Shape, storage shape, and data type of a tensor. This object is held by Python code and can be used as the return value of `infer_meta`.

Note: This type differs from [`ge.graph.TensorDesc`](../../graph/TensorDesc/overview.md). `ge.graph.TensorDesc` is used for the tensor description on the graph construction side, containing Format, OriginShape, and the corresponding `get_*`/`set_*` interfaces; `ge.runtime.TensorDesc` is used only for the metadata input and output of the Python prototype.

The following example creates two input TensorDesc objects and calls `infer_meta` to return the output TensorDesc.

```python
from ge.custom_op import register_op
from ge.graph import DataType
from ge.runtime import TensorDesc


@register_op(op_type="AddCustom")
def infer_meta(x: TensorDesc, y: TensorDesc) -> TensorDesc:
    return TensorDesc(x.shape, x.data_type)


x = TensorDesc([2, 3], DataType.DT_FLOAT)
y = TensorDesc([2, 3], DataType.DT_FLOAT)
z = infer_meta(x, y)
```
