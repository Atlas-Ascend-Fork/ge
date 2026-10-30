# listen\_ip\_info

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:40:47.973Z pushedAt=2026-10-29T03:25:50.819Z nl=2 -->

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

Sets the cluster listening information on the PROMPT side, corresponding to the underlying llm.listenIpInfo configuration item.

## Prototype

```python
listen_ip_info(listen_ip_info)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| listen_ip_info | str | Set to the IP address and port on the Device side. One or more entries can be configured. When multiple entries are configured, separate them with semicolons.<br><br>  - In the single-process single-card scenario, configure one entry, for example: "192.168.1.1:26000"<br>  - In the single-process multi-card scenario, configure multiple entries, for example: "192.168.1.1:26000;192.168.1.2:26000" |

## Examples

```python
from llm_datadist import LLMConfig
llm_config = LLMConfig()
# Setting method for single-process single-card
llm_config.listen_ip_info = "192.168.1.1:26000"
# Setting method for single-process multi-card
# llm_config.listen_ip_info = "192.168.1.1:26000;192.168.1.2:26000"
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown if the parameters are incorrect.

## Constraints

None
