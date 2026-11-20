# AllocTensorMsg (MetaContext Class)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-13T10:09:53.591Z pushedAt=2026-11-16T06:11:08.683Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Requests a Tensor-type msg based on shape and data type. This function is called by [Proc](Proc.md).

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

Pointer to the allocated Tensor.

## Exception Handling

If the Tensor pointer cannot be obtained, NULL is returned.

## Constraints

None.
