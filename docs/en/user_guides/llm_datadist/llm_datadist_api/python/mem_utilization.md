# mem\_utilization

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:56:08.972Z pushedAt=2026-11-17T11:29:07.890Z nl=2 -->

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

Configures the utilization of the ge.flowGraphMemMaxSize memory. The default value is 0.95.

## Prototype

```python
mem_utilization(mem_utilization)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| mem_utilization | float | Memory utilization. Default value: 0.95. Value range: 0.0 to 1.0. |

## Examples

```python
from llm_datadist import LLMConfig
llm_config = LLMConfig()
llm_config.mem_utilization = 0.95
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None.
