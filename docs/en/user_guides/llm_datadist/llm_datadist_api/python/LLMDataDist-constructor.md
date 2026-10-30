# LLMDataDist-constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:43:49.601Z pushedAt=2026-10-29T03:25:50.830Z nl=2 -->

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

Constructs an LLMDataDist instance.

## Prototype

```python
__init__(role: LLMRole, cluster_id: int)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| role | [LLMRole](LLMRole.md) | Cluster role. The value can be as follows.<br><br>  - LLMRole.DECODER: incremental cluster, which can only be used as a Client<br>  - LLMRole.PROMPT: full cluster, which can only be used as a Server<br>  - LLMRole.MIX: hybrid deployment |
| cluster_id | int | Cluster ID. It is the LLMDataDist identifier and must be unique within the scope of all participants in link establishment. |

## Examples

```python
from llm_datadist import LLMDataDist, LLMRole
llm_datadist = LLMDataDist(LLMRole.DECODER, 0)
```

## Return Value

Returns an LLMDataDist instance in normal cases.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
