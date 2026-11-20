# DeallocateCache

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:50:46.410Z pushedAt=2026-11-17T03:39:12.095Z nl=2 -->

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

Releases the cache.

## Prototype

```cpp
Status DeallocateCache(int64_t cache_id)
```

## Parameters

| Parameter | Input/Output | Value Description |
| --- | --- | --- |
| cache_id | Input | ID of the cache. |

## Examples

```cpp
Status ret = llm_datadist.DeallocateCache(cache.cache_id);
```

## Return Value

- LLM\_SUCCESS: Success
- Others: Failure

## Constraints

Before calling this API, call [Initialize](Initialize.md) to complete initialization.
