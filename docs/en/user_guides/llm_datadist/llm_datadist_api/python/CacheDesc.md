# CacheDesc

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:30:11.222Z pushedAt=2026-10-29T03:25:50.772Z nl=2 -->

## Function

Constructs a CacheDesc, which is typically used as the parameter type in the [allocate\_cache](allocate_cache.md) API of KvCacheManager.

## Prototype

```python
__init__(self,
                 num_tensors: int,
                 shape: Union[Tuple[int], List[int]],
                 data_type: DataType,
                 placement: Placement = Placement.DEVICE,
                 batch_dim_index: int = 0,
                 seq_len_dim_index: int = -1,
                 kv_tensor_format: str = None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| num_tensors | int | Number of tensors in the cache. |
| shape | Union[Tuple[int], List[int]] | Shape of the tensor. |
| data_type | [DataType](DataType.md) | Data type of the tensor. |
| placement | [Placement](Placement.md) | Device type where the cache resides. The default value is Placement.DEVICE. |
| batch_dim_index | int | Dimension of the batch size in the shape. The default value is 0, indicating dimension 0. |
| seq_len_dim_index | int | Dimension of seq_len in the shape. The default value is -1, indicating that it is not configured. |
| kv_tensor_format | str | Format of the cache. |

## Examples

```python
from llm_datadist import CacheDesc
cache_desc = CacheDesc(80, [4, 2048, 1, 128], DataType.DT_FLOAT16)
```

## Return Value

Returns a CacheDesc instance under normal conditions.

Throws a TypeError or ValueError exception when the input data type is incorrect.

## Constraints

None
