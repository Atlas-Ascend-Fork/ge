# remote\_cluster\_id

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-29T03:31:59.131Z pushedAt=2026-10-29T06:42:47.352Z nl=2 -->

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
