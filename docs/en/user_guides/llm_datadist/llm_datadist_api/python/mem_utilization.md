# mem\_utilization

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:29:29.685Z pushedAt=2026-10-29T06:37:54.990Z nl=2 -->

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
