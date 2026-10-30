# allocate\_cache

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:27:22.500Z pushedAt=2026-10-29T03:25:50.762Z nl=2 -->

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

Allocates a cache. After successful allocation, the cache is referenced by both cache\_id and cache\_keys. The resources occupied by the cache are actually released only after all these references are removed.

The reference of cache\_id must be removed through [deallocate\_cache](deallocate_cache.md), while the reference of cache\_keys can be removed in either of the following two ways.

- The Decoder removes it after successfully calling the [pull\_cache](pull_cache.md) API.
- The Prompt removes it when calling the [remove\_cache\_key](remove_cache_key.md) API.

## Prototype

```python
allocate_cache(cache_desc: CacheDesc, cache_keys: Union[Tuple[CacheKey], List[CacheKey]] = ())
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache_desc | [CacheDesc](CacheDesc.md) | Description of the cache. |
| cache_keys | Union[Tuple[[CacheKey](CacheKey.md)], List[[CacheKey](CacheKey.md)]] | Can be set only when LLMRole is PROMPT. It is used to pull KV during decoding. |

## Examples

```python
from llm_datadist import *
...
kv_cache_manager = data_dist.kv_cache_manager
cache_desc = CacheDesc(80, [2, 2 * 1024 * 1024], DataType.DT_FLOAT16)
cache_keys = [CacheKey(prompt_cluster_id=0, req_id=1)]
kv_cache = kv_cache_manager.allocate_cache(cache_desc, cache_keys)
```

## Return Value

KvCache is returned in normal cases.

TypeError or ValueError may be thrown for invalid parameters.

If the cache\_keys contains the CacheKey bound during memory allocation, an [LLMException](LLMException.md) exception is thrown.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) exception is thrown.

## Constraints

- When cache\_keys is passed in, if the Cache batch size\>1, the same number of CacheKeys must be provided, each referencing a group of kv tensors.
- If the batch of the current inference is not fully occupied, that is, there are invalid batch\_index, a special CacheKey (with req\_id set to UINT64\_MAX) must be inserted as a placeholder. If the idle batch\_index is at the end, it can be omitted.
- If cache\_keys contains duplicates, the last one takes effect.
- This API does not support concurrent calls.
