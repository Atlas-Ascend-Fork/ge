# copy\_cache

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:43:26.795Z pushedAt=2026-11-17T10:21:06.377Z nl=2 -->

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

Copy the KV cache.

## Prototype

```python
copy_cache(dst: KvCache, src: KvCache, dst_batch_index: int = 0, src_batch_index: int = 0, offset: int = 0, size: int = -1, req_id: Optional[int] = None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| dst | [KvCache](KvCache-constructor.md) | Target cache. |
| src | [KvCache](KvCache-constructor.md) | Source cache. |
| dst_batch_index | int | batch_index of the destination cache. The default value is 0. |
| src_batch_index | int | batch_index of the source cache. The default value is 0. |
| offset | int | Offset of each tensor. The default value is 0. |
| size | int | Set to an integer greater than 0, indicating the size to copy.<br>Or set to -1, indicating a full copy.<br>The default value is -1. |
| req_id | Optional[int] | req_id associated with this call. If this parameter is set, the req_id is printed in the diagnostic logs related to the local call.<br>The default value is None. |

## Examples

```python
kv_cache_manager.copy_cache(dst_cache, src_cache, 0, 1, 0, 128)
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) is thrown.

## Constraints

- The CacheDesc of src and dst must match.
- This API does not support concurrency. Concurrent calls are queued and wait.
