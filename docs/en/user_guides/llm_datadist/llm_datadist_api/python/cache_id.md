# cache\_id

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:30:07.309Z pushedAt=2026-10-29T03:25:50.771Z nl=2 -->

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

Obtain the ID of KvCache.

## Prototype

```python
@property
cache_id() -> int
```

## Parameters

None

## Examples

```python
...
kv_cache = kv_cache_manager.allocate_cache(cache_desc, cache_keys)
print(kv_cache.cache_id)
```

## Return Value

In normal cases, the return type is the ID of KvCache.

## Constraints

None
