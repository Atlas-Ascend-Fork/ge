# link\_clusters

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:40:31.246Z pushedAt=2026-10-29T03:25:50.817Z nl=2 -->

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

Link establishment is initiated by the Client side. Since most of the dynamically scaled components are on the Decode side, P is defined as the Server side and D as the Client side. The link establishment process is implemented as a flow in which D initiates link establishment to P.

## Prototype

```python
link_clusters(clusters: Union[List[LLMClusterInfo], Tuple[LLMClusterInfo]], timeout=3000)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| clusters | Union[List[[LLMClusterInfo](LLMClusterInfo-constructor.md)], Tuple[[LLMClusterInfo](LLMClusterInfo-constructor.md)]] | Cluster list. |
| timeout | int | Timeout, in ms. The default timeout is 3000 ms. |

## Examples

```python
from llm_datadist import LLMDataDist, LLMRole, LLMStatusCode, LLMClusterInfo
llm_datadist = LLMDataDist(LLMRole.DECODER, 0)
cluster = LLMClusterInfo()
cluster.remote_cluster_id = 1
cluster.append_local_ip_info("1.1.1.1", 26000)
cluster.append_remote_ip_info("1.1.1.1", 26000)
ret, rets = llm_datadist.link_clusters([cluster], 5000)
if ret != LLMStatusCode.LLM_SUCCESS:
    raise Exception("link failed.")
for cluster_i in range(len(rets)):
    link_ret = rets[cluster_i]
    if link_ret != LLMStatusCode.LLM_SUCCESS:
        print(f"{cluster_i} link failed.")
```

## Return Value

Normally, a tuple of two values is returned. The first value is the return value of the API, of the type [LLMStatusCode](LLMStatusCode.md), and the second is a list of the link establishment results of each cluster, of the type [LLMStatusCode](LLMStatusCode.md).

A TypeError or ValueError may be thrown for invalid parameters.

## Constraints

- The requirements for link establishment are as follows. Establishing too many links poses performance risks of memory OOM and KV Cache transfer.
  - On the Server side (P side), the number of concurrent link establishments = 16, and the maximum number of communication links allowed = 512.
  - On the Client side (D side), the maximum number of communication links allowed = 512.

- It is recommended to configure a timeout of 200 ms or more.
- Only the Client initiates the call.
