# AllocateCache

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:45:45.980Z pushedAt=2026-11-17T06:05:37.450Z nl=2 -->

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

Allocates a cache.

## Prototype

```cpp
Status AllocateCache(const CacheDesc &cache_desc, Cache &cache)
```

## Parameters

| Parameter | Input/Output | Value Description |
| --- | --- | --- |
| cache_desc | Input | Description of the Cache. |
| cache | Output | Cache allocated. It is valid only when this API returns LLM_SUCCESS. |

## Examples

```cpp
CacheDesc kv_desc{};
kv_desc.num_tensors = 80;
kv_desc.data_type = DT_FLOAT16;
kv_desc.shape = {1, 256};
Cache cache;
Status ret = llm_datadist.AllocateCache(kv_desc, cache);
```

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Parameter error
- LLM\_DEVICE\_OUT\_OF\_MEMORY: Device memory is insufficient
- Others: Failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization.

This interface takes effect only when placement in the parameter cache\_desc is CachePlacement::kDevice. For details about Cache, see [CacheDesc](CacheDesc.md).
