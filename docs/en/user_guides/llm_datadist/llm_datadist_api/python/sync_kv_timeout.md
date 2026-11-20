# sync\_kv\_timeout

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T04:00:56.608Z pushedAt=2026-11-17T11:32:20.390Z nl=2 -->

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

Configures the timeout for a series of APIs, corresponding to the underlying llm.SyncKvCacheWaitTime configuration item.

## Prototype

```python
sync_kv_timeout(sync_kv_timeout)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| sync_kv_timeout | int or str | Timeout for synchronizing kv, in ms. The default value is 1000 ms if not configured. |

## Examples

```python
from llm_datadist import LLMConfig
llm_config = LLMConfig()
llm_config.sync_kv_timeout = 1000
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
