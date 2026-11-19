# ge\_options

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:47:17.686Z pushedAt=2026-11-17T10:25:07.192Z nl=2 -->

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

Configures additional GE configuration items.

## Prototype

```python
ge_options(ge_options)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| ge_options | dict[str, str] | Configures GE options.<br>Among them, ge.flowGraphMemMaxSize is important. It indicates the maximum memory occupied by all KV caches. If it is set too large, the available memory of the model is compressed. Specify it based on the actual situation. |

## Examples

```python
from llm_datadist import LLMConfig
ge_options = {
    "ge.flowGraphMemMaxSize": "4106127360"
}
llm_config = LLMConfig()
llm_config.ge_options = ge_options
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
