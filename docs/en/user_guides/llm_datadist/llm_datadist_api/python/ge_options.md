# ge\_options

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:36:23.166Z pushedAt=2026-10-29T03:25:50.798Z nl=2 -->

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

Configures additional GE options.

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

No value is returned in normal cases.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
