# LinkLlmClusters

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:19:55.450Z pushedAt=2026-10-29T03:25:50.737Z nl=2 -->

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

Establishes links between devices.

## Prototype

```cpp
Status LinkLlmClusters(const std::vector<ClusterInfo> &clusters, std::vector<Status> &rets, int32_t timeout_in_millis = 1000)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| clusters | Input | Cluster information for which links need to be established. The type is [ClusterInfo](ClusterInfo-and-IpInfo.md). |
| rets | Output | Link establishment result of each cluster. |
| timeout_in_millis | Input | Link establishment timeout, in ms. The default timeout is 1000. |

## Return Value

- LLM\_SUCCESS: The API returns success only when links are established successfully for all clusters.
- Others: Link establishment fails. Check the link establishment result of each cluster in rets.

## Exception Handling

- LLM\_PROCESSING\_LINK: The API is protected by a lock. When multiple threads of an LLM-DataDist call the link establishment API, the calls are executed serially. If the waiting time of another thread exceeds the configured timeout, an error is reported and the thread exits.
- LLM\_ALREADY\_LINK: The incremental clusters have already established links with all clusters.
- LLM\_LINK\_FAILED: Link establishment fails.
- LLM\_CLUSTER\_NUM\_EXCEED\_LIMIT: The number of clusters for link establishment exceeds the upper limit, which is currently 16.

## Constraints

It must be called after the [Initialize](Initialize.md) API initialization is complete, and can be called only when the role is Decoder.
