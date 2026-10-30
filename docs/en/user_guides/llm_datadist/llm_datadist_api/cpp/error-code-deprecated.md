# error-code (deprecated)

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:15:29.233Z pushedAt=2026-10-29T03:25:50.727Z nl=2 -->

The error-code definition files are stored in `${INSTALL_DIR}/include/ge/llm_error_codes.h` and `${INSTALL_DIR}/include/llm_datadist/llm_datadist.h`. `${INSTALL_DIR\} Replace the path with the actual file storage path after software installation. If the Ascend-cann-toolkit package is installed, taking root installation as an example, the file storage path after installation is: /usr/local/Ascend/ascend-toolkit/latest.

The error-code is defined by the following macro.

```cpp
namespace ge {
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_WAIT_PROC_TIMEOUT, 1);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_KV_CACHE_NOT_EXIST, 2);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_REPEAT_REQUEST, 3);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_REQUEST_ALREADY_COMPLETED, 4);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_PARAM_INVALID, 5);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_ENGINE_FINALIZED, 6);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_NOT_YET_LINK, 7);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_ALREADY_LINK, 8);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_LINK_FAILED, 9);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_UNLINK_FAILED, 10);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_NOTIFY_PROMPT_UNLINK_FAILED, 11);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_CLUSTER_NUM_EXCEED_LIMIT, 12);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_PROCESSING_LINK, 13);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_DEVICE_OUT_OF_MEMORY, 14);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_PREFIX_ALREADY_EXIST, 15);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_PREFIX_NOT_EXIST, 16);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_SEQ_LEN_OVER_LIMIT, 17);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_NO_FREE_BLOCK, 18);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_BLOCKS_OUT_OF_MEMORY, 19);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_CACHE_INCOMPATIBLE, 20);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_CACHE_KEY_ALREADY_EXIST, 21);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_COPY_CACHE_FAILED, 22);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_CACHE_ID_ALREADY_EXIST, 23);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_EXIST_LINK, 24);
GE_ERRORNO_DEFINE(0b01, 0b01, 0b000, 8, 11, LLM_FEATURE_NOT_ENABLED, 25);
}  // namespace ge
```

The meanings of the specific error-codes are as follows.

| error-code | Meaning | Possible Cause | Recoverable | Solution |
| --- | --- | --- | --- | --- |
| 4294967295 | failed! | - | No | Preserve the site, obtain the Host/Device logs, and back them up (Device logs need to be exported back to the host via port). |
| 1343270913 | request wait to be processed timeout! | - The batch waiting times out.<br>  - The request is not scheduled within the timeout period. | Yes | 1. The upper layer calls complete for processing.<br>  2. The upper layer controls the load. |
| 1343270914 | KV does not exist! | The incremental side obtains the KV from the full side, but the KV does not exist. | Yes | 1. Check Whether the clusterid and req_id are correct.<br>  2. Check Whether it is a Prompt timeout, and confirm whether the KV has been released.<br>  3. Check Whether there is duplicate pulling. |
| 1343270915 | repeat request! | The same req_id is called multiple times in a single iteration. | Yes | Check Whether there are duplicate calls. |
| 1343270916 | request already complete! | When scheduled, the upper layer has already sent complete. | Yes | Troubleshoot whether the complete operation is called for unprocessed tokens. |
| 1343270917 | Parameter's invalid! | Invalid parameters, including LLM & GE parameter validation. The specific cause needs to be confirmed through logs, and this generally occurs during the debugging phase. | Yes | Troubleshoot the cause based on logs. |
| 1343270918 | llm engine finalized! | When scheduled, the upper layer has already sent Finalize. | No | When the upper layer calls, Troubleshoot whether all requests are processed before exit. |
| 1343270919 | decoder cluster is not linked with prompt! | Decode is not linked with Prompt. | Yes | The upper layer Troubleshoots the linking status between Decode and Prompt. |
| 1343270920 | decoder cluster is already linked with prompt cluster! | Decode and Prompt are already linked. | Yes | The upper layer Troubleshoots the linking status between Decode and Prompt. |
| 1343270921 | decoder cluster link with prompt cluster failed! | Decode and Prompt fail to be linked. | Yes | 1. When the LinkClusters API returns this error-code, check the linking result of each cluster in the API output parameters.<br>  2. When this error-code exists in the rets output parameter of the LinkClusters API, Troubleshoot the network connection between clusters.<br><br>When linking fails, automatic rollback is performed to ensure that resources are not abnormally occupied. |
| 1343270922 | decoder cluster unlink with prompt cluster failed! | Decode and Prompt fail to be unlinked (Link Failure). | Yes | 1. When the LinkClusters API returns this error-code, check the unlinking result of each cluster in the API output parameters.<br>  2. When this error-code exists in the rets output parameter of the LinkClusters API, Troubleshoot the network connection between clusters.<br><br>When unlinking fails, no rollback is performed. |
| 1343270923 | decoder cluster notify prompt cluster to unlink failed! | Decode fails to notify Prompt to unlink (Link Failure). | Yes | 1. Troubleshoot the network connection between Decode and Prompt.<br>  2. Actively call the UnlinkClusters API on the Prompt side to clean up residual resources. |
| 1343270924 | cluster num exceeds limit! | The clusters passed in a single API call exceed the upper limit, which is currently 16. | Yes | Troubleshoot the API input parameters. The number of clusters cannot exceed 16. |
| 1343270925 | link is currently processing, try again later! | A link or unlink operation is currently in progress. | Yes | Try again later. |
| 1343270926 | device out of memory! | The full KV Cache memory is insufficient. | Yes | After the incremental side finishes pulling the KV, call LLMReqComplete to end the request. |
| 1343270927 | Prefix has already existed. | The Common Prefix already exists. | Yes | Do not repeatedly send requests with the same Common Prefix. |
| 1343270928 | Prefix does not exist. | The Common Prefix does not exist. | Yes | Regenerate the Common Prefix before sending. |
| 1343270929 | Sequence length exceeds limit. | In the PagedAttention scenario, the sentence length exceeds the block space size. | Yes | Troubleshoot the API input parameters. The request sentence length cannot exceed block_num*block_size. |
| 1343270930 | No free block. | There is no free block in the system. | Yes | The upper layer can choose to recompute or release existing requests. |
| 1343270931 | Block is out of memory. | The pre-allocated Blocks memory is insufficient. | Yes | Additionally check whether the total size configuration of the data plane mbuf memory is reasonable. |
