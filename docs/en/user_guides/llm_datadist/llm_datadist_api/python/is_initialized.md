# is\_initialized

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:49:11.604Z pushedAt=2026-11-17T10:36:13.913Z nl=2 -->

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
