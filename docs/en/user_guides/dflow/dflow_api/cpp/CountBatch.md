# CountBatch

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-21T11:44:37.249Z pushedAt=2026-10-28T12:30:42.467Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Feature Overview

The CountBatch feature groups multiple data items into a batch based on the UDF for a compute processing point according to batch\_size.

This feature applies to DataFlow asynchronous scenarios, as described below.

- When no data is input for a long time, you can set a timeout period through the CountBatch feature. If padding is not set, the currently available data is sent to the compute processing point for processing after the timeout.
- After the timeout period is set, if the data does not reach batch\_size, you can set the padding attribute through the CountBatch feature. The compute point pads the data to batch\_size according to the padding setting and then outputs it.

## Usage

When constructing a DataFlow graph, users add the CountBatch feature by setting attributes on the input of a FlowNode. The following is an example.

```cpp
CountBatch count_batch = {};
// Set the value of each attribute in count_batch as required.
count_batch.timeout = 10;
count_batch.batch_size = 300;
count_batch.padding = true;
count_batch.slide_stride = 5;
DataFlowInputAttr flow_attr = {DataFlowAttrType::COUNT_BATCH, &count_batch};
std::vector<DataFlowInputAttr> flow_attrs = {flow_attr};
// Set it through MapInput of the FlowNode.
FlowNode::MapInput(xx, xx, xx, flow_attrs);
```

| Attribute Name | Data Type | Value Description |
| --- | --- | --- |
| batch_size | int64_t | Batch size. |
| timeout | int64_t | This parameter takes effect only when batch_size is set.<br>Batch waiting time, in ms. Value range: [0, 4294967295). The default value is 0, which means waiting until the batch is full. |
| padding | bool | This parameter takes effect only when batch_size and timeout are set.<br>Whether to pad when the batch is not full. The default value is false, which means no padding. |
| slide_stride | int64_t | This parameter takes effect only when batch_size is set.<br>Sliding window stride. Value range: [0, batch_size].<br><br>  - A value greater than 0 and less than batch_size enables the sliding window mode for batching.<br>  - If not set, it equals 0.<br>  - A value equal to batch_size means batching without a sliding window stride.<br>  - A value greater than batch_size reports an error. |
| batch_dim | int64_t | Not enabled. |
| flag | int32_t | Not enabled. |
| drop_remainder | bool | Not enabled. |

## Precautions

The current Batch feature cannot perform load balancing. Therefore, in a 2P environment, you need to add \{"ge.exec.logicalDeviceClusterDeployMode", "SINGLE"\}, \{"ge.exec.logicalDeviceId", "\[0:0\]"\} during GE initialization.
