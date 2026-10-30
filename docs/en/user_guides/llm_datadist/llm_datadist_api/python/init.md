# init

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:37:58.105Z pushedAt=2026-10-29T03:25:50.804Z nl=2 -->

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

Initializes LLMDataDist. A link is established unilaterally, that is, the Client initiates the link establishment to the Server, with Prompt acting as the Server and Decode acting as the Client. KV can only be pulled from Decode to Prompt.

## Prototype

```python
init(options: Dict[str, str])
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| options | Dict[str, str] | Configuration items.<br>The options to be passed in can be generated through [LLMConfig](LLMConfig.md).<br>Required fields: device_id and listen_ip_info (listen_ip_info is required because Prompt acts as the Server; Decode, acting as the Client, does not need to fill in listen_ip_info). |

## Examples

```python
from llm_datadist import LLMDataDist, LLMRole, LLMConfig
llm_datadist = LLMDataDist(LLMRole.PROMPT, 0)
llm_config = LLMConfig()
llm_config.listen_ip_info = "192.168.1.1:26000"
llm_config.device_id = 0
engine_options = llm_config.generate_options()
llm_datadist.init(engine_options)
```

## Return Value

No value is returned in normal cases.

In abnormal cases, [LLMException](LLMException.md) is thrown.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

After successful initialization, [finalize](finalize.md) must be called before the system exits.
