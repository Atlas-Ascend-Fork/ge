# Finalize

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:52:16.158Z pushedAt=2026-11-17T03:39:12.098Z nl=2 -->

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
