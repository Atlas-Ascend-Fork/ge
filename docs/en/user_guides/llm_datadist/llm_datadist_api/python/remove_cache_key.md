# remove\_cache\_key

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:32:32.726Z pushedAt=2026-10-29T06:43:45.277Z nl=2 -->

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

Removes a CacheKey. This API can be called only when LLMRole is PROMPT.

After the CacheKey is removed, the cache can no longer be pulled by [pull\_cache](pull_cache.md).

## Prototype

```python
remove_cache_key(cache_key: CacheKey)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache_key | [CacheKey](CacheKey.md) | CacheKey to be removed. |

## Examples

```python
cache_keys = [CacheKey(prompt_cluster_id=0, req_id=1)]
kv_cache_manager.remove_cache_key(cache_keys[0])
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) is thrown.

## Constraints

- If the CacheKey does not exist or has been removed, this operation is a no-op.
- This API does not support concurrent calls.
