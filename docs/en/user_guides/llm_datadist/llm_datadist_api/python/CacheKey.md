# CacheKey

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:30:51.528Z pushedAt=2026-10-29T03:25:50.774Z nl=2 -->

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

Constructs a CacheKey, which is typically used as the parameter type in the [allocate\_cache](allocate_cache.md) and [pull\_cache](pull_cache.md) APIs of KvCacheManager.

## Prototype

```python
__init__(*args, **kwargs)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| prompt_cluster_id | int | ID of the remote cluster where the cache resides. Mandatory. |
| req_id | int | req_id associated with the cache. Mandatory. |
| model_id | int | model_id associated with the cache. Defaults to 0. |
| prefix_id | int | Common prefix ID associated with the cache. Defaults to 2^64 - 1. |

## Examples

```python
from llm_datadist import CacheKey
cache_key = CacheKey(0, 1, 0)
```

## Return Value

Returns a CacheKey instance under normal conditions.

Throws a TypeError or ValueError exception when the data type of the input is incorrect.

## Constraints

None
