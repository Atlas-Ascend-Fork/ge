# LLMClusterInfo-constructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:52:53.752Z pushedAt=2026-11-17T11:27:28.327Z nl=2 -->

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

Constructs LLMClusterInfo, which is used as the parameter type of the [link\_clusters](link_clusters.md) and [unlink\_clusters](unlink_clusters.md) APIs.

## Prototype

```python
__init__()
```

## Parameters

None

## Examples

```python
from llm_datadist import LLMClusterInfo
llm_cluster = LLMClusterInfo()
```

## Return Value

Returns an instance of LLMClusterInfo.

## Constraints

None
