# LLMConfig-constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:42:38.660Z pushedAt=2026-10-29T03:25:50.827Z nl=2 -->

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

Constructs an LLMConfig. Calling the [init](init.md) API requires a configuration dictionary. To simplify configuration, you can use this class to construct the configuration dictionary.

## Prototype

```python
__init__()
```

## Parameters

None

## Examples

```python
from llm_datadist import LLMConfig
llm_config = LLMConfig()
```

## Return Value

Returns an instance of LLMConfig.

## Constraints

None
