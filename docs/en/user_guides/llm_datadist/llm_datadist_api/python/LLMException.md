# LLMException

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:55:15.305Z pushedAt=2026-11-17T11:28:24.211Z nl=2 -->

When calling the LLMDataDist APIs, the LLMException exception may be thrown in abnormal scenarios. Currently, this class has only one API status\_code.

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

Obtains the error-code of the exception. For the error-code list, see [LLMStatusCode](LLMStatusCode.md).

## Prototype

```python
status_code()
```

## Parameters

None

## Examples

```python
from llm_datadist import *
...
cache_keys = [CacheKey(1, req_id=1), CacheKey(1, req_id=2)]
try:
    kv_cache_manager.pull_cache(cache_keys[0], cache, 0)
except LLMException as exe:
    print(exe.status_code)
```

## Return Value

Returns the error-code.

## Constraints

None
