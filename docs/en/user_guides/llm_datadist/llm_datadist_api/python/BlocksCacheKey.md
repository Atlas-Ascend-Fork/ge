# BlocksCacheKey

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T10:06:44.652Z pushedAt=2026-11-17T03:39:12.133Z nl=2 -->

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

Constructs a BlocksCacheKey, which is typically used as a parameter type in the [allocate\_blocks\_cache](allocate_blocks_cache.md) and [pull\_blocks](pull_blocks.md) APIs of KvCacheManager.

## Prototype

```python
__init__(*args, **kwargs)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| prompt_cluster_id | int | ID of the remote cluster where the cache resides. Mandatory. |
| model_id | int | model_id associated with the cache. Defaults to 0. |

## Examples

```python
from llm_datadist import BlocksCacheKey
blocks_cache_key = BlocksCacheKey(0, 0)
```

## Return Value

Returns an instance of BlocksCacheKey in normal cases.

If the input data type is incorrect, a TypeError or ValueError exception is thrown.

## Constraints

None
