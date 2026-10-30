# TransferWithCacheKeyConfig

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:38:00.120Z pushedAt=2026-10-29T06:47:37.936Z nl=2 -->

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

Constructs TransferWithCacheKeyConfig.

## Prototype

```python
__init__(cache_key: Union[BlocksCacheKey, CacheKeyByIdAndIndex], src_layer_range: range = None, dst_layer_range: range = None, src_batch_index: int = 0)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache_key | Union[[BlocksCacheKey](BlocksCacheKey.md), [CacheKeyByIdAndIndex](CacheKeyByIdAndIndex.md)] | cluster_id of the instance where the destination cache resides. |
| src_layer_range | range | Mandatory parameter. Range of layers to be transferred locally. Only step 1 is supported. |
| dst_layer_range | range | Mandatory parameter. Range of layers to be transferred on the remote end. Only step 1 is supported. |
| src_batch_index | int | Batch index of the local cache. It can be set when the source cache is in a non-PA scenario. |

## Examples

```python
from llm_datadist import TransferWithCacheKeyConfig
TransferWithCacheKeyConfig(BlocksCacheKey(1), range(0, 40), range(0, 40))
```

## Return Value

Normally, an instance of TransferWithCacheKeyConfig is returned.

If a parameter is invalid, TypeError, ValueError, or [LLMException](LLMException.md) may be thrown.

## Constraints

- src\_layer\_range indicates that the range must be equal to the range indicated by dst\_layer\_range.
- When cache\_key is BlocksCacheKey, src\_batch\_index can only be 0.
