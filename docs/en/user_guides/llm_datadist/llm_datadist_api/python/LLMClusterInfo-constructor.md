# LLMClusterInfo-constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:41:46.080Z pushedAt=2026-10-29T06:18:44.398Z nl=2 -->

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
