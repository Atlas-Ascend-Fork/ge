# CacheKeyByIdAndIndex

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:31:08.376Z pushedAt=2026-10-29T03:25:50.775Z nl=2 -->

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

If the data type of the input is incorrect, a TypeError or ValueError exception is thrown.

## Constraints

None
