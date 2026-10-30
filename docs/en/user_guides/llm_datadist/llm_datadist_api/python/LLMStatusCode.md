# LLMStatusCode

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-29T03:29:50.889Z pushedAt=2026-10-29T06:37:49.901Z nl=2 -->

The enum class corresponding to status\_code in LLMException. The enum values and solutions are listed in the following table.

| Enum Value | Description | Recoverable | Solution |
| --- | --- | --- | --- |
| LLM_SUCCESS | Success | None | None |
| LLM_FAILED | General failure | No | Restart the machine or container.<br>Preserve the site, obtain the Host/Device logs, and back them up. |
| LLM_WAIT_PROCESS_TIMEOUT | Processing timeout | Yes | - If this error is reported by transfer-related APIs such as [pull_cache](pull_cache.md), [pull_blocks](pull_blocks.md), and [transfer_cache_async](transfer_cache_async.md), the link is unrecoverable and must be re-established.<br>  - If this error is reported by other APIs, increase the timeout period and retry. |
| LLM_PARAM_INVALID | Invalid parameter | Yes | Troubleshoot the cause based on logs. |
| LLM_KV_CACHE_NOT_EXIST | KV does not exist | Yes | - Check whether the request in the corresponding full-side error log is complete.<br>  - Check whether there are duplicate pulls.<br>  - Check whether the parameters marking the target cache are incorrect. |
| LLM_REPEAT_REQUEST | Duplicate request | Yes | Check whether there are duplicate calls. |
| LLM_NOT_YET_LINK | Link not established | Yes | Check the Decode and Prompt link establishment at the upper layer. |
| LLM_ALREADY_LINK | Link already established | Yes | Check the Decode and Prompt link establishment at the upper layer. |
| LLM_LINK_FAILED | Link establishment failed | Yes | When this error-code is present in the second return value of [link_clusters](link_clusters.md), check the network connection between the corresponding clusters. |
| LLM_UNLINK_FAILED | Link disconnection failed | Yes | When this error-code is present in the second return value of [unlink_clusters](unlink_clusters.md), check the network connection between the corresponding clusters. |
| LLM_NOTIFY_PROMPT_UNLINK_FAILED | Failed to notify the Prompt side of link disconnection | Yes | 1. Check the network connection between Decode and Prompt.<br>  2. Actively call [unlink_clusters](unlink_clusters.md) on the Prompt side to clean up residual resources. |
| LLM_CLUSTER_NUM_EXCEED_LIMIT | Number of clusters exceeds the limit | Yes | Check the parameters passed to [link_clusters](link_clusters.md) and [unlink_clusters](unlink_clusters.md). The number of clusters cannot exceed 16. |
| LLM_PROCESSING_LINK | Link establishment in progress | Yes | A link establishment or disconnection operation is currently in progress. Try again later. |
| LLM_PREFIX_ALREADY_EXIST | Prefix already exists | Yes | Check whether a common prefix with the same Prefix Id has already been loaded. If so, release it first. |
| LLM_PREFIX_NOT_EXIST | Prefix does not exist | Yes | Check whether the Prefix Id in the request has been loaded. |
| LLM_DEVICE_OUT_OF_MEMORY | Device memory insufficient | Yes | Check whether the requested memory has not been released. |
| LLM_EXIST_LINK | Unreleased links exist during switch_role | Yes | Check whether [unlink_clusters](unlink_clusters.md) has been called to disconnect all links before switching the role of the current LLMDataDist. |
| LLM_FEATURE_NOT_ENABLED | Feature not enabled | Yes | Check whether the required option is passed when initializing LLMDataDist.<br>If this error is thrown when switching the role of the current LLMDataDist, check whether enable_switch_role = True is set in LLMConfig during initialization. |
| LLM_LINK_BUSY | Link busy | Yes | Check whether the APIs called concurrently conflict with each other. For example, this error-code is reported when the following APIs are called concurrently.<br><br>  - Calling [pull_cache](pull_cache.md) and [transfer_cache_async](transfer_cache_async.md) of KvCacheManager concurrently on the same link.<br>  - Calling [check_link_status](check_link_status.md) and [pull_cache](pull_cache.md) of KvCacheManager concurrently. |
| LLM_OUT_OF_MEMORY | Insufficient memory | Yes | This error-code occurs only in CacheManager mode.<br>Check whether the memory pool is large enough to accommodate the requested KV size.<br>Check whether the requested memory has not been released. |
| LLM_DEVICE_MEM_ERROR | Error virtual address of a memory UCE (a memory error that the system hardware cannot directly handle and recover) | Yes | Refer to the description of the torch_npu.npu.restart_device API in *TorchNPU Custom API* to obtain and fix the error virtual address of the memory UCE.<br> Note: This error-code is reserved and not supported yet. |
| LLM_SUSPECT_REMOTE_ERROR | Suspected UCE memory fault | No | The upper-layer framework needs to combine other faults to comprehensively determine whether it is a UCE memory fault or another fault. |
| LLM_UNKNOWN_ERROR | Unknown error | No | Preserve the site, obtain Host/Device logs, and back them up. |
