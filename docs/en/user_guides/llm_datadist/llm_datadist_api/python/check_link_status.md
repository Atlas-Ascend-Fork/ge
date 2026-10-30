# check\_link\_status

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:32:27.679Z pushedAt=2026-10-29T03:25:50.780Z nl=2 -->

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

Quickly checks whether the link status is normal.

## Prototype

```python
check_link_status(remote_cluster_id: int)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| remote_cluster_id | int | Remote cluster ID. |

## Examples

```python
from llm_datadist import LLMDataDist, LLMRole, LLMStatusCode, LLMClusterInfo
...
try:
    data_dist.check_link_status(remote_cluster_id=0)
except LLMException as ex:
    print(f"check_link_status exception:{ex.status_code}")
    raise ex
kv_cache_manager = data_dist.kv_cache_manager
...
kv_cache_manager.pull_cache(prompt_cache_key, local_kv_cache, batch_index=0)
```

## Return Value

No return value in normal cases.

If the operation fails, an [LLMException](LLMException.md) exception is thrown.

Parameter errors may throw TypeError or ValueError.

## Constraints

Only the client side can call this API.

If the call fails and the error-code in the exception is an unrecoverable error-code, re-establish the link.

If the call fails, you must keep calling this API until it succeeds before you can call APIs such as [pull\_cache](pull_cache.md) and [pull\_blocks](pull_blocks.md).

If this API runs concurrently with APIs such as [pull\_cache](pull_cache.md) and [pull\_blocks](pull_blocks.md), a [LLMException](LLMException.md) exception may be thrown, with the error-code being LLM\_LINK\_BUSY.

The timeout period is specified by the [llm.SyncKvCacheWaitTime](sync_kv_timeout.md) configuration item.
