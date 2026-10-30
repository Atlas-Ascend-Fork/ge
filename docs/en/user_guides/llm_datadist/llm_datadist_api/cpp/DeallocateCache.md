# DeallocateCache

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:14:11.943Z pushedAt=2026-10-29T03:25:50.722Z nl=2 -->

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

Deallocates the cache.

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
