# allocate\_blocks\_cache

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:26:09.928Z pushedAt=2026-10-29T03:25:50.758Z nl=2 -->

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

In the PagedAttention scenario, allocate Cache for multiple blocks. After the Cache is allocated successfully, the memory can be released through [deallocate\_cache](deallocate_cache.md).

## Prototype

```python
allocate_blocks_cache(cache_desc: CacheDesc, blocks_cache_key: Optional[BlocksCacheKey] = None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache_desc | [CacheDesc](CacheDesc.md) | Description of the Cache. |
| blocks_cache_key | Optional[[BlocksCacheKey](BlocksCacheKey.md)] | Can be set only when LLMRole is PROMPT. It is used to pull KV during Decode. |

## Examples

```python
from llm_datadist import BlocksCacheKey
num_blocks = 1000
cache_desc = CacheDesc(80, [num_blocks , 128 * 1024], DataType.DT_FLOAT16)
blocks_cache_key = BlocksCacheKey(0, 0)
kv_cache = kv_cache_manager.allocate_blocks_cache(cache_desc, blocks_cache_key)
```

## Return Value

KvCache is returned in normal cases.

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) exception is thrown.

## Constraints

This API does not support concurrent calls.
