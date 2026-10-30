# Finalize

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:16:14.841Z pushedAt=2026-10-29T03:25:50.731Z nl=2 -->

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

Resource release API of LLM-DataDist.

## Prototype

```cpp
void Finalize()
```

## Parameters

None

## Return Value

None

## Exception Handling

None

## Constraints

- It must be used together with [Initialize](Initialize.md).
- In separate deployment, when the incremental cluster pulls KV data from the full cluster, ensure that the full cluster calls this API only after the KV data has been synchronized; otherwise, the operation fails.
- In separate deployment, when the full cluster is synchronizing KV data to the incremental cluster, ensure that the incremental cluster calls this API only after the full cluster has finished synchronizing the KV data; otherwise, the operation fails.
- This API cannot be called concurrently with other APIs.
