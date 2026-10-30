# per\_device\_tensor\_addrs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:29:32.381Z pushedAt=2026-10-29T06:38:05.470Z nl=2 -->

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

Obtains the address of KvCache.

## Prototype

```python
@property
per_device_tensor_addrs() -> List[List[int]]
```

## Parameters

None

## Examples

```python
...
kv_cache = kv_cache_manager.allocate_cache(cache_desc, cache_keys)
# The two-level List structure is designed for single-process multi-card scenarios. In single-process single-card scenarios, directly obtain the address of the first card.
print(kv_cache.per_device_tensor_addrs[0])
```

## Return Value

In normal cases, the return type is the address of KVCache.

## Constraints

None
