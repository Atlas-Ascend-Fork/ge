# swap\_blocks

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:33:57.914Z pushedAt=2026-10-29T06:43:53.713Z nl=2 -->

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

Swaps data in and out between cpu\_cache and npu\_cache.

For the swap out function, this API starts four threads to execute parallel tasks; for the swap in function, it starts one d2d thread. For stable performance, process core binding is recommended.

The swap in function consists of two phases: H2D and D2D. To ensure performance, this API allocates a buffer of four blocks for pipelined copying. Therefore, it is recommended that you reserve the corresponding Device memory to prevent OOM.

## Prototype

```python
swap_blocks(src: KvCache, dst: KvCache, src_to_dst: Dict[int, int])
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| src | [KvCache](KvCache-constructor.md) | Source cache. |
| dst | [KvCache](KvCache-constructor.md) | Target cache. |
| src_to_dst | Dict[int, int] | The dict content represents (source block index, target block index) |

## Examples

```python
from llm_datadist import KvCache
...
npu_cache = kv_cache_manager.allocate_blocks_cache(npu_cache_desc, npu_cache_key)
cpu_cache = KvCache.create_cpu_cache(cpu_cache_desc, cpu_addrs) # cpu_addrs comes from the created cpu tensors
# swap in
kv_cache_manager.swap_blocks(cpu_cache, npu_cache, {1:2, 3:4})
# swap out
kv_cache_manager.swap_blocks(npu_cache, cpu_cache, {1:2, 3:4})
```

## Return Value

No value is returned under normal conditions.

If the input data type is incorrect, or the source cache and target cache do not match, a TypeError or ValueError exception is thrown.

If the input parameter is None, an AttributeError exception is thrown.

## Constraints

None
