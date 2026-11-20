# cache\_desc

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T10:07:50.077Z pushedAt=2026-11-17T03:39:12.136Z nl=2 -->

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

Obtains the description of KvCache.

## Prototype

```python
@property
cache_desc() -> CacheDesc
```

## Parameters

None

## Examples

```python
...
kv_cache = kv_cache_manager.allocate_cache(cache_desc, cache_keys)
print(kv_cache.cache_desc.num_tensors)
```

## Return Value

In normal cases, the returned cache description is of the KvCache type.

## Constraints

None
