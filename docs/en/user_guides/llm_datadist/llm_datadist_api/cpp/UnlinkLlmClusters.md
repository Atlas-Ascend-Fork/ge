# UnlinkLlmClusters

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:24:45.758Z pushedAt=2026-10-29T03:25:50.753Z nl=2 -->

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

Disconnects the link between devices.

## Prototype

```cpp
Status UnlinkLlmClusters(const std::vector<ClusterInfo> &clusters, std::vector<Status> &rets, int32_t timeout_in_millis = 1000, bool force_flag = false)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| clusters | Input | Information about the clusters to be disconnected. The type is [ClusterInfo](ClusterInfo-and-IpInfo.md). |
| rets | Output | Disconnection result of each cluster. |
| timeout_in_millis | Input | Timeout for disconnection, in ms. The default timeout is 1000. |
| force_flag | Input | Whether to forcibly disconnect the link. The default is no.<br>Forcible disconnection only forcibly tears down the local link, and both ends must call this API.<br>Non-forcible disconnection is initiated on the Decode side. When no fault occurs, the links on both ends are torn down. In a link fault scenario, it takes a long time, and the call must also be initiated on the Prompt side. |

## Return Value

- SUCCESS: The API returns success only when all clusters are disconnected successfully.
- Others: The disconnection fails. Check the disconnection result of each cluster in rets.

## Exception Handling

- LLM\_PROCESSING\_LINK: The API is protected by a lock. When multiple threads of an LLM-DataDist call the disconnection API, the calls are executed serially. If the waiting time of another thread exceeds the configured timeout, an error is reported and the thread exits.
- LLM\_UNLINK\_FAILED: The disconnection fails.

## Constraints

This API must be called after the [Initialize](Initialize.md) API is initialized.
