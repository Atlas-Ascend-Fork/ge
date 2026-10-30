# dataflow.CountBatch

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:07:12.188Z pushedAt=2026-10-29T03:00:43.643Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

The CountBatch feature groups multiple data items into a batch based on batch\_size for UDF-based computation processing points. This feature applies to dataflow asynchronous scenarios, as described below.

- When no data is input for a long time, you can set a timeout period through the CountBatch feature. If padding is not set, the currently available data is sent to the compute processing point for processing after the timeout.
- After the timeout period is set, if the data does not reach batch\_size, you can set the padding attribute through the CountBatch feature. The compute point pads the data to batch\_size according to the padding setting and then outputs it.

## Prototype

```python
CountBatch(batch_size=0, slide_stride=0, timeout=0, padding=False)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| batch_size | int64_t | Batch size. |
| timeout | int64_t | This parameter takes effect only when batch_size is set.<br>Batch waiting time, in ms. Value range: [0, 4294967295). The default value is 0, which means waiting until the batch is full. |
| padding | Bool | This parameter takes effect only when batch_size and timeout are set.<br>Whether to pad when the batch is not full. The default value is false, which means no padding. |
| slide_stride | int64_t | This parameter takes effect only when batch_size is set.<br>Sliding window stride. Value range: [0, batch_size].<br><br>  - A value greater than 0 and less than batch_size enables the sliding window mode for batching.<br>  - If not set, it equals 0. A value equal to batch_size means batching without a sliding window stride.<br>  - A value greater than batch_size reports an error. |

## Return Value

Returns None in normal scenarios.

Returns "TypeError" if the parameter type is incorrect.

## Examples

```python
import dataflow as df
# Set the attribute values of count_batch as needed and pass them directly through the constructor
count_batch = df.CountBatch(batch_size=300, slide_stride=5,timeout=10,padding=300)
# Create count_batch first and then set its values
count_batch = df.CountBatch()
count_batch.batch_size = 300
# Use it through the map_input API of FlowNode
df.FlowNode(...).map_input(..., [count_batch])
```

## Constraints

The current CountBatch feature cannot perform load balancing. Therefore, in a 2P environment, you need to add \{"ge.exec.logicalDeviceClusterDeployMode", "SINGLE"\}, \{"ge.exec.logicalDeviceId", "\[0:0\]"\} during dataflow.init initialization.
