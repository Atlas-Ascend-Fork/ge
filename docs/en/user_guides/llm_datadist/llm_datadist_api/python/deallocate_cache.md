# deallocate\_cache

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:34:10.634Z pushedAt=2026-10-29T03:25:50.790Z nl=2 -->

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

Releases the cache.

If a CacheKey is associated with the cache during allocation, the actual release is deferred until all CacheKeys are pulled or [remove\_cache\_key](remove_cache_key.md) is executed.

## Prototype

```python
deallocate_cache(cache: KvCache)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache | [KvCache](KvCache-constructor.md) | KV cache to be released. |

## Examples

```python
kv_cache_manager.deallocate_cache(kv_cache)
```

## Return Value

No value is returned in normal cases.

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) exception is thrown.

## Constraints

- If the KvCache does not exist or has already been released, this operation is a no-op.
- This API does not support concurrent calls.
