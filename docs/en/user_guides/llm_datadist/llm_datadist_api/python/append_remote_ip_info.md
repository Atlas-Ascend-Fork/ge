# append\_remote\_ip\_info

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T10:05:54.013Z pushedAt=2026-11-17T03:39:12.131Z nl=2 -->

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

Adds remote cluster IP information.

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

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
