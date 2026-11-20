# remote\_cluster\_id

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:58:19.177Z pushedAt=2026-11-17T11:31:14.538Z nl=2 -->

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

Sets the peer cluster ID.

## Prototype

```python
remote_cluster_id(remote_cluster_id)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| remote_cluster_id | int | Peer cluster ID. |

## Examples

```python
llm_cluster = LLMClusterInfo()
llm_cluster.remote_cluster_id = 1
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
