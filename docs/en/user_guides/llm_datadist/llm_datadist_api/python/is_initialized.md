# is\_initialized

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:38:28.324Z pushedAt=2026-10-29T03:25:50.806Z nl=2 -->

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

Queries whether the KvCacheManager instance has been initialized.

## Prototype

```python
is_initialized() -> bool
```

## Parameters

NA

## Examples

```python
from llm_datadist import LLMDataDist
llm_datadist = LLMDataDist(LLMRole.Decode, 0)
...
llm_datadist.init(engine_options)
kv_cache_manager = llm_datadist.kv_cache_manager
is_initialized = kv_cache_manager.is_initialized()
```

## Return Value

Returns True if LLMDataDist has been initialized.

Returns False if LLMDataDist has been destroyed.

## Constraints

If this API returns False, do not call any API of [KvCacheManager](KvCacheManager.md).
