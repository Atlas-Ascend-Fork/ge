# create\_cpu\_cache

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:43:49.094Z pushedAt=2026-11-17T10:22:12.816Z nl=2 -->

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

Creates a CPU cache.

## Prototype

```python
create_cpu_cache(cache_desc: CacheDesc, addrs: Union[List[int], List[List[int]]])
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache_desc | [CacheDesc](CacheDesc.md) | Description of the cache. |
| addrs | Union[List[int], List[List[int]]] | Address of the CPU cache. |

## Examples

```python
from llm_datadist import KvCache
cpu_addrs = []
# Configure the addresses of multiple cards in single-process multi-card mode.
# cpu_addrs = [[]]
cpu_cache = KvCache.create_cpu_cache(cpu_cache_desc, cpu_addrs) # cpu_addrs comes from the created CPU tensors
```

## Return Value

On success, **cpu\_cache** of the **KvCache** type is returned.

If the input data type is incorrect, a TypeError or ValueError exception is thrown.

If the input parameter is None, an AttributeError exception is thrown.

## Constraints

None
