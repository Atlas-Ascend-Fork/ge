# error-code

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:15:58.089Z pushedAt=2026-10-29T03:25:50.729Z nl=2 -->

error-code is defined by the following macro.

```cpp
namespace llm_datadist {
constexpr Status LLM_SUCCESS = 0x0U;
constexpr Status LLM_FAILED = 0xFFFFFFFFU;
constexpr Status LLM_WAIT_PROC_TIMEOUT = 0x5010B001U;
constexpr Status LLM_KV_CACHE_NOT_EXIST = 0x5010B002U;
constexpr Status LLM_PARAM_INVALID = 0x5010B005U;
constexpr Status LLM_NOT_YET_LINK = 0x5010B007U;
constexpr Status LLM_ALREADY_LINK = 0x5010B008U;
constexpr Status LLM_LINK_FAILED = 0x5010B009U;
constexpr Status LLM_UNLINK_FAILED = 0x5010B00AU;
constexpr Status LLM_NOTIFY_PROMPT_UNLINK_FAILED = 0x5010B00BU;
constexpr Status LLM_CLUSTER_NUM_EXCEED_LIMIT = 0x5010B00CU;
constexpr Status LLM_PROCESSING_LINK = 0x5010B00DU;
constexpr Status LLM_DEVICE_OUT_OF_MEMORY = 0x5010B00EU;
constexpr Status LLM_EXIST_LINK = 0x5010B018U;
constexpr Status LLM_FEATURE_NOT_ENABLED = 0x5010B019U;
constexpr Status LLM_TIMEOUT = 0x5010B01AU;
constexpr Status LLM_LINK_BUSY = 0x5010B01BU;
constexpr Status LLM_OUT_OF_MEMORY = 0x5010B01CU;
}  // namespace llm_datadist
```

The specific error-code meanings are as follows.

| enum value | Description | recoverable | Solution |
| --- | --- | --- | --- |
| LLM_SUCCESS | Success | None | None |
| LLM_FAILED | General failure | No | Preserve the site, obtain Host/Device logs, and back them up. |
| LLM_WAIT_PROC_TIMEOUT | Processing timeout | Yes | - If this error is reported by transfer-related APIs such as [PullKvCache](PullKvCache.md) and [PullKvBlocks](PullKvBlocks.md), the link is unrecoverable and must be re-established.<br>  - If this error is reported by other APIs, increase the timeout and retry. |
| LLM_KV_CACHE_NOT_EXIST | KV does not exist | Yes | - Check whether cache_id is correct.<br>  - Check whether the Cache has been released.<br>  - Check whether the request in the corresponding full-side error log is complete.<br>  - Check whether there is duplicate pulling. |
| LLM_PARAM_INVALID | Invalid parameter | Yes | Troubleshoot the cause based on logs. |
| LLM_NOT_YET_LINK | Link not established | Yes | Check the Decode and Prompt link establishment at the upper layer. |
| LLM_ALREADY_LINK | Duplicate link establishment | Yes | Check the Decode and Prompt link establishment at the upper layer. |
| LLM_LINK_FAILED | Link establishment failed | Yes | When this error-code is present in the second return value of [LinkLlmClusters](LinkLlmClusters.md), check the network connection between the corresponding clusters. |
| LLM_UNLINK_FAILED | Link disconnection failed | Yes | When this error-code is present in the second return value of [UnlinkLlmClusters](UnlinkLlmClusters.md), check the network connection between the corresponding clusters. |
| LLM_NOTIFY_PROMPT_UNLINK_FAILED | Failed to notify the Prompt side of link disconnection | Yes | 1. Check the network connection between Decode and Prompt.<br>  2. Actively call the [UnlinkLlmClusters](UnlinkLlmClusters.md) API on the Prompt side to clean up residual resources. |
| LLM_CLUSTER_NUM_EXCEED_LIMIT | Number of clusters exceeds the limit | Yes | Check the parameters passed to [LinkLlmClusters](LinkLlmClusters.md) and [UnlinkLlmClusters](UnlinkLlmClusters.md). The number of clusters cannot exceed 16. |
| LLM_PROCESSING_LINK | Link establishment in progress | Yes | A link establishment or disconnection operation is currently in progress. Try again later. |
| LLM_DEVICE_OUT_OF_MEMORY | Device memory insufficient | Yes | Check whether the requested memory has not been released. |
| LLM_EXIST_LINK | Unreleased link exists when setting the role | Yes | Check whether [UnlinkLlmClusters](UnlinkLlmClusters.md) has been called to disconnect all links before SetRole. |
| LLM_FEATURE_NOT_ENABLED | Feature not enabled | Yes | Check whether the required option is passed when initializing LLM-DataDist.<br>If this error is reported during SetRole, check whether OPTION_ENABLE_SET_ROLE = "1" is set during initialization. |
| LLM_TIMEOUT | Processing timeout | No | Preserve the site, obtain Host/Device logs, and back them up. |
| LLM_LINK_BUSY | Link busy | Yes | Reserved error-code, not returned for now. |
| LLM_OUT_OF_MEMORY | Insufficient memory | Yes | Check whether the memory pool or system memory is sufficient. |
