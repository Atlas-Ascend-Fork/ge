# copy\_blocks

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:43:23.065Z pushedAt=2026-11-17T10:20:51.194Z nl=2 -->

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

Copies blocks in the PagedAttention scenario.

## Prototype

```python
copy_blocks(cache: KvCache, copy_block_info: Dict[int, List[int]])
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| cache | [KvCache](KvCache-constructor.md) | Target cache. |
| copy_block_info | Dict[int, List[int]] | The dict content represents (source block index, list of target block indexes). |

## Examples

```python
from llm_datadist import *
...
kv_cache_manager.copy_blocks(kv_cache, {1: [2,3]})
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

If the execution time exceeds the [sync\_kv\_timeout](sync_kv_timeout.md) configuration, an [LLMException](LLMException.md) is thrown.

## Constraints

This API does not support concurrency. Concurrent calls are queued and wait.
