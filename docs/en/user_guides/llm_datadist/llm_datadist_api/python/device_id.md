# device\_id

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:35:11.444Z pushedAt=2026-10-29T03:25:50.791Z nl=2 -->

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

Sets the device ID of the current process, corresponding to the underlying ge.exec.deviceId configuration item.

## Prototype

```python
device_id(device_id)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| device_id | Union[int, List[int], Tuple[int]] | Sets the device ID of the current process. It can be configured as a single value or a list. When configured as a list, separate the values with half-width commas.<br><br>  - In a single-process single-card scenario, configure a single value, for example: 0<br>  - In a single-process multi-card scenario, configure a list, for example: [0, 1] |

## Examples

```python
from llm_datadist import LLMConfig
llm_config = LLMConfig()
# Set the device ID in a single-process single-card scenario.
llm_config.device_id = 0
# Set the device ID in a single-process multi-card scenario.
# llm_config.device_id = [0, 1]
```

## Return Value

No value is returned in normal cases.

TypeError or ValueError may be raised when the parameters are invalid.

## Constraints

None
