# get\_tensor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:59:54.251Z pushedAt=2026-10-29T03:13:53.339Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the tensor object in FlowMsg.

## Prototype

```python
get_tensor() -> dataflow.Tensor
```

## Parameters

None

## Return Value

Returns an object of the dataflow.Tensor type.

## Exception Handling

None

## Constraints

If FlowMsg is empty, tensor returns None.
