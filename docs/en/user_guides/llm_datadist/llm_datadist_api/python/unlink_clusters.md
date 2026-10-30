# unlink\_clusters

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:38:28.890Z pushedAt=2026-10-29T06:48:09.587Z nl=2 -->

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

Disconnects links.

## Prototype

```python
unlink_clusters(clusters: Union[List[LLMClusterInfo], Tuple[LLMClusterInfo]], timeout=3000, force=False)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| clusters | Union[List[[LLMClusterInfo](LLMClusterInfo-constructor.md)], Tuple[[LLMClusterInfo](LLMClusterInfo-constructor.md)]] | Cluster list. |
| timeout | int | Timeout, in ms. The default timeout is 3000 ms. |
| force | bool | Whether to forcibly disconnect. The default value is False. True indicates forcible disconnection.<br><br>  - Forcible disconnection only forcibly tears down the local link, and both ends must call it.<br>  - Non-forcible disconnection is initiated in Decode. When no fault occurs, the links on both ends are torn down. When a fault causes the disconnection to fail, the disconnection operation must also be initiated on the Prompt end.<br>  - When no fault occurs, the links on both ends are torn down.<br>  - When a fault causes the disconnection to fail, the disconnection operation must also be initiated on the Prompt end. |

## Examples

```python
from llm_datadist import LLMDataDist, LLMRole, LLMStatusCode, LLMClusterInfo
llm_datadist = LLMDataDist(LLMRole.DECODER, 0)
cluster = LLMClusterInfo()
cluster.remote_cluster_id = 1
cluster.append_local_ip_info("1.1.1.1", 26000)
cluster.append_remote_ip_info("1.1.1.1", 26000)
ret, rets = llm_datadist.unlink_clusters([cluster], 5000)
if ret != LLMStatusCode.LLM_SUCCESS:
    raise Exception("unlink failed.")
for cluster_i in range(len(rets)):
    unlink_ret = rets[cluster_i]
    if unlink_ret != LLMStatusCode.LLM_SUCCESS:
        print(f"{cluster_i} unlink failed.")
```

## Return Value

Normally, a tuple of two values is returned. The first value is the return value of the API, of the type [LLMStatusCode](LLMStatusCode.md), and the second is a list of the link establishment results of each cluster, of the type [LLMStatusCode](LLMStatusCode.md).

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
