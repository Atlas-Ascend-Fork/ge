# AllocTensorMsg (MetaContext Class)

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:40:05.179Z pushedAt=2026-10-28T12:30:42.448Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Applies for a Tensor-type msg based on the shape and data type. This function is called by [Proc](Proc.md).

## Prototype

```cpp
std::shared_ptr<FlowMsg> AllocTensorMsg(const std::vector<int64_t> &shape, TensorDataType dataType)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| Shape | Input | Shape of the Tensor. |
| dataType | Input | Data type of the Tensor. |

## Return Value

Pointer to the applied Tensor.

## Exception Handling

If no Tensor pointer can be obtained, NULL is returned.

## Constraints

None.
