# LayerSynchronizer

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:40:24.248Z pushedAt=2026-10-29T03:25:50.816Z nl=2 -->

An abstract class that users need to inherit to implement the related interfaces. Currently, this class has only one interface synchronize\_layer.

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

Waits for the specified layer of the model to finish execution. Users need to inherit LayerSynchronizer and implement this interface.

This interface is called when KvCacheManager executes [transfer\_cache\_async](transfer_cache_async.md). When this interface returns success, the transfer of the current layer cache starts.

## Prototype

```python
synchronize_layer(layer_index: int, timeout_in_millis: Optional[int]) -> bool
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| layer_index | int | Index of the layer |
| timeout_in_millis | Optional[int] | Timeout duration. Not supported yet. |

## Examples

This interface is not called directly by users. Instead, it is called as a callback by [KvCacheManager](transfer_cache_async.md).

## Return Value

Under normal circumstances, returns whether the synchronization succeeds.

If the parameters are invalid, TypeError or ValueError may be thrown.

## Constraints

None
