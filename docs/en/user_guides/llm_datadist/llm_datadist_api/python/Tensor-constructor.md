# Tensor-constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:35:57.258Z pushedAt=2026-10-29T06:46:34.527Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 inference products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 training products: Not supported
<!-- end id3 -->

## Function

Constructs a Tensor.

## Prototype

```python
__init__(data, tensor_desc: TensorDesc = None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| data | Union[np.ndarray, Tensor] | Data of the Tensor. |
| tensor_desc | [TensorDesc](TensorDesc-constructor.md) | Description of the Tensor. |

## Examples

```python
from llm_datadist import Tensor
tensor = Tensor(numpy.array([1]))
```

## Return Value

Returns a Tensor instance on success.

Throws RuntimeError when the passed data information does not match the tensor\_desc information.

## Constraints

None
