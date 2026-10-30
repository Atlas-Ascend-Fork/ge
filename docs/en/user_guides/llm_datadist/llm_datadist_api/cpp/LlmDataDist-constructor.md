# LLMDataDist-constructor

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:20:39.742Z pushedAt=2026-10-29T03:25:50.740Z nl=2 -->

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

Creates an LLM-DataDist object.

## Prototype

```cpp
LlmDataDist(uint64_t cluster_id, LlmRole role)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| cluster_id | Input | Cluster ID. The LlmDataDist identifier, which must be unique within the scope of all participants in link establishment. |
| role | Input | Type is [LlmRole](LlmRole.md), used to identify the current role. |

## Return Value

None

## Exception Handling

None

## Constraints

None
