# register\_op

## 产品支持情况

全量芯片支持。

## 功能说明

用于注册自定义算子原型，并通过被装饰函数的类型标注描述算子输入、属性和输出。

## 函数原型

```python
register_op(*, op_type: str, mutates_args=(), output_names=None) -> callable
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :--- | :--- |
| op_type | 输入 | 自定义算子类型。类型为非空字符串。同一`op_type`只能对应一个原型；重复注册完全相同的原型时复用已有注册，注册不同原型时抛出`ValueError`。 |
| mutates_args | 输入 | 输出引用输入关系的声明，可选参数。支持按输出顺序声明的字符串列表或元组，也支持由输入名称和输出索引组成的二元组列表或元组。默认值为`()`。两种形式不能混用。 |
| output_names | 输入 | 输出名称，可选参数，类型为列表或元组。按算子输出顺序指定名称，名称个数须与返回类型标注声明的输出个数一致。默认值为`None`，表示使用自动命名。输出与输入同名时表示引用该输入。 |

`mutates_args`支持以下两种形式：

- 顺序形式：`("x", "y")`表示输出索引0（第1个输出）引用输入`x`，输出索引1（第2个输出）引用输入`y`。
- 显式形式：`(("x", 1),)`表示输出索引1（第2个输出）引用输入`x`。

输出按顺序引用输入时，推荐使用顺序形式；需要跳过某些输出时，可以使用显式形式指定输出索引。

## 返回值说明

返回函数装饰器。装饰器完成原型注册后返回原函数，不改变原函数的调用方式，并为原函数设置`__ge_op_proto_descriptor__`属性。

- `op_type`、被装饰对象、函数签名、类型标注、属性默认值、`mutates_args`格式或`output_names`类型不合法时，抛出`TypeError`。
- `output_names`中的名称为空或含空字符时，抛出`TypeError`；名称数量与输出个数不匹配或名称重复时，抛出`ValueError`。
- `mutates_args`存在重复、越界或不存在的输入名称，或者原型注册发生冲突时，抛出`ValueError`。
- `output_names`和`mutates_args`同时设置时，抛出`ValueError`。

## 约束说明

被装饰函数是自定义算子的Meta推导函数，其签名用于声明算子输入（Input）、属性（Attr）和输出（Output）。注册时对被装饰函数进行以下校验：

- 被装饰对象必须是Python函数，不支持可变位置参数和可变关键字参数。
- 输入（Input）：位置参数表示算子输入，必须提供类型标注且不能设置默认值，类型仅支持`TensorDesc`、`Optional[TensorDesc]`和`List[TensorDesc]`。
- 属性（Attr）：`*`后的仅限关键字参数表示算子属性，必须提供类型标注，类型仅支持`int`、`float`、`bool`、`str`、`DataType`、`Tensor`、`List[int]`、`List[float]`、`List[bool]`、`List[str]`、`List[DataType]`和`List[List[int]]`。属性默认值必须与类型标注严格匹配，`Tensor`属性不能设置默认值。
- 输出（Output）：返回类型标注表示算子输出。必须提供返回类型标注，类型仅支持`None`、`TensorDesc`、`List[TensorDesc]`，以及由`TensorDesc`和`List[TensorDesc]`组成的`Tuple`。
- `mutates_args`引用的输入名称必须存在。每个输入名称和输出索引最多使用一次，输出索引不能越界，顺序形式和显式形式不能混用。
- `output_names`中的每个名称必须是非空且不含空字符的字符串，输出名称不能重复。
- `mutates_args`和`output_names`不支持同时设置。
- 同一函数重复注册相同的原型时复用已有注册；原型内容发生变化或其他函数使用已注册的`op_type`时，注册失败。

## 调用示例

以下示例注册`AddCustom`算子原型。`x`和`y`为必选输入。返回类型标注表示算子有一个输出，默认名称为`output0`。

```python
from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="AddCustom")
def infer_add(x: TensorDesc, y: TensorDesc) -> TensorDesc:
    return x
```

以下示例注册`InplaceAddCustom`算子原型。`x`和`y`为必选输入，`alpha`为可选属性，默认值为`1.0`。返回类型标注表示算子有一个输出。`mutates_args=("x",)`表示第1个输出引用输入`x`，输出名称为`x`。

```python
from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="InplaceAddCustom", mutates_args=("x",))
def infer_meta(
    x: TensorDesc,
    y: TensorDesc,
    *,
    alpha: float = 1.0,
) -> TensorDesc:
    return x
```

以下示例注册`NamedOutputsCustom`算子原型。`x`为必选输入。返回类型标注声明三个输出，`output_names`将其依次命名为`value`、`parts`和`x`。其中，`parts`为一组动态输出，输出`x`引用同名输入。

```python
from typing import List, Tuple

from ge.custom_op import register_op
from ge.runtime import TensorDesc


@register_op(op_type="NamedOutputsCustom", output_names=("value", "parts", "x"))
def infer_named_outputs(
    x: TensorDesc,
) -> Tuple[TensorDesc, List[TensorDesc], TensorDesc]:
    return x, [x], x
```
