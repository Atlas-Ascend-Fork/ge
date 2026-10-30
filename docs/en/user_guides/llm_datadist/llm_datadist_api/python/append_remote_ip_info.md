# append\_remote\_ip\_info

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:27:41.579Z pushedAt=2026-10-29T03:25:50.765Z nl=2 -->

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

Adds the IP information of a remote cluster.

## Prototype

```python
append_remote_ip_info(self, ip: Union[str, int], port: int)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| ip | Union[str, int] | Set to the IP address of the Device card in the peer cluster. |
| port | int | Set to the port of the Device card in the peer cluster. |

## Examples

```python
llm_cluster = LLMClusterInfo()
llm_cluster.append_remote_ip_info("1.1.1.1", 10000)
```

## Return Value

No value is returned in normal cases.

TypeError or ValueError may be thrown if the parameters are invalid.

## Constraints

None
