# finalize

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:46:31.909Z pushedAt=2026-11-17T10:24:46.244Z nl=2 -->

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

Releases LLMDataDist.

## Prototype

```python
finalize()
```

## Parameters

None

## Examples

```python
from llm_datadist import LLMDataDist, LLMRole, LLMConfig
llm_datadist = LLMDataDist(LLMRole.DECODER, 0)
llm_config = LLMConfig()
llm_config.listen_ip_info = "192.168.1.1:26000"
llm_config.device_id = 0
engine_options = llm_config.generate_options()
llm_datadist.init(engine_options)
llm_datadist.finalize()
```

## Return Value

None

## Constraints

After successful initialization, call [finalize](finalize.md) before the system exits.

[finalize](finalize.md) must not be called concurrently with other APIs.
