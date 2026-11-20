# LLMDataDist-constructor

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:56:24.271Z pushedAt=2026-11-17T06:30:44.431Z nl=2 -->

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
| cluster_id | Input | Cluster ID. The LlmDataDist identifier must be unique within the scope of all participants in link establishment. |
| role | Input | Type is [LlmRole](LlmRole.md), used to identify the current role. |

## Return Value

None

## Exception Handling

None

## Constraints

None
