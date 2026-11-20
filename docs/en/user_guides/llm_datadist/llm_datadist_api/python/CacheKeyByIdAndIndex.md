# CacheKeyByIdAndIndex

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:41:49.281Z pushedAt=2026-11-17T10:19:17.706Z nl=2 -->

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

Constructs a CacheKeyByIdAndIndex instance, which is typically used as a parameter type in the [pull\_cache](pull_cache.md) API.

## Prototype

```python
__init__(self, cluster_id: int, cache_id: int, batch_index=0)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cluster_id | int | Cluster ID of the node where the cache resides. |
| cache_id | int | Cache ID associated with the cache. |
| batch_index | int | Batch index of the cache. The default value is 0. |

## Examples

```python
from llm_datadist import CacheKeyByIdAndIndex
cache_key = CacheKeyByIdAndIndex(0, 1, 0)
```

## Return Value

On success, returns an instance of CacheKeyByIdAndIndex.

If the input data type is incorrect, a TypeError or ValueError exception is thrown.

## Constraints

None
