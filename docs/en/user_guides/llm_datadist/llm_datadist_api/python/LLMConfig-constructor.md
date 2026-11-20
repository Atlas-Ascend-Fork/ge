# LLMConfig-constructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:54:03.898Z pushedAt=2026-11-17T11:27:45.288Z nl=2 -->

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
