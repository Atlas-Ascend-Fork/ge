# buf\_pool\_cfg

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:29:17.233Z pushedAt=2026-10-29T03:25:50.769Z nl=2 -->

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

Users specify the memory level configuration to improve memory allocation performance and utilization.

## Prototype

```python
buf_pool_cfg(buf_pool_cfg)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| buf_pool_cfg | string | A string in JSON array format, containing three nodes: total_size, blk_size, and max_buf_size |

| Configuration Item | Optional/Mandatory | Description |
| --- | --- | --- |
| total_size | Required | Size of the memory pool at the current level, in bytes<br>Constraints:<br>total_size is a multiple of 2M and a multiple of blk_size, and its maximum value must not exceed 0xFFFFFFFF. |
| blk_size | Required | Minimum memory value that can be allocated at a time at the current level, in bytes<br>Constraints:<br>It must satisfy 2^n, be within (0, 2M], and be less than or equal to max_buf_size. |
| max_buf_size | Required | Maximum memory value that can be allocated at a time at the current level, in bytes<br>Constraints: It must be less than total_size. If multiple levels are configured, max_buf_size must be strictly increasing in the order in which the levels appear. |

## Examples

```python
from llm_datadist import LLMConfig
llm_config = LLMConfig()
llm_config.buf_pool_cfg= '{"buf_cfg": [{"total_size":2097152,"blk_size":256,"max_buf_size":8192},{"total_size": 10485760,"blk_size": 8192,"max_buf_size": 8388608},{"total_size": 69206016,"blk_size": 8192,"max_buf_size": 67108864}]}'
```

## Return Value

No value is returned in normal cases.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
