# Initialize

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:52:59.423Z pushedAt=2026-11-17T06:27:47.732Z nl=2 -->

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

Initializes LLM-DataDist.

## Prototype

```cpp
Status Initialize(const std::map<AscendString, AscendString> &options)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| options | Input | Initialization parameter values. For details, see Table 1. |

**Table 1** options

| Parameter | Optional/Required | Description |
| --- | --- | --- |
| OPTION_LISTEN_IP_INFO | Required for Pormpt | IP address and port on the Device side.<br>Configuration example: "192.168.1.1:26000". In a single-process multi-card scenario, separate multiple values with semicolons. |
| OPTION_DEVICE_ID | Required | Sets the Device ID of the current process, for example, "0". In a single-process multi-card scenario, separate multiple values with semicolons. |
| OPTION_SYNC_CACHE_WAIT_TIME | Optional | Timeout for KV-related operations, in ms. If not configured, the default value is 1000 ms. The related APIs are as follows.<br><br>  - [AllocateCache](AllocateCache.md)<br>  - [DeallocateCache](DeallocateCache.md)<br>  - [PullKvCache](PullKvCache.md)<br>  - [PullKvBlocks](PullKvBlocks.md)<br>  - [CopyKvCache](CopyKvCache.md)<br>  - [CopyKvBlocks](CopyKvBlocks.md)<br>  - [PushKvCache](PushKvCache.md)<br>  - [PushKvBlocks](PushKvBlocks.md) |
| OPTION_BUF_POOL_CFG | Optional | Configures the memory pool size and level information to improve memory allocation performance and utilization. The format is a JSON string. The field meanings are described in Table 2, and an example is as follows.<br>{<br>"buf_cfg":[{"total_size":2097152,"blk_size":256,"max_buf_size":8192}],<br>"buf_pool_size": 2147483648<br>} |
| OPTION_ENABLE_SET_ROLE | Optional | Configures whether smooth role switching is supported. The values are as follows.<br><br>  - 1: Supported.<br>  - 0: Not supported. If not configured, the default value is not supported.<br><br>Related API: [SetRole](SetRole.md). |

**Table 2** OPTION\_BUF\_POOL\_CFG configuration

| Configuration Item | Optional/Required | Description |
| --- | --- | --- |
| buf_cfg | Optional | Memory pool level configuration. For details, see Table 3. |
| buf_pool_size | Optional | Memory pool size, in bytes. |

**Table 3** buf\_cfg configuration

| Configuration Item | Optional/Required | Description |
| --- | --- | --- |
| total_size | Required | Size of the memory pool at the current level, in bytes.<br> Note: total_size must be a multiple of 2M and a multiple of blk_size. The maximum value must not exceed 0xFFFFFFFF. |
| blk_size | Required | Minimum memory size that can be requested at a time at the current level, in bytes.<br> Note: The value must satisfy 2^n, fall within (0, 2M], and be less than or equal to max_buf_size. |
| max_buf_size | Required | Maximum memory size that can be requested at a time at the current level, in bytes.<br> Note: The value must be less than total_size, and max_buf_size must remain strictly increasing. |

## Return Value

- LLM\_SUCCESS: Success
- LLM\_PARAM\_INVALID: Parameter error
- Others: Failure

## Exception Handling

None

## Constraints

It must be used in pairs with [Finalize](Finalize.md). After successful initialization, [Finalize](Finalize.md) must be called before any exit to ensure resource release. Otherwise, resources may be released in an unexpected order, causing issues.
