# TimeBatch

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:55:36.229Z pushedAt=2026-10-29T02:39:40.077Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Feature Overview

The TimeBatch feature is based on UDF.

A normal model processes one data item at a time. When a batch of data needs to be processed at once, the data must be grouped into a batch. The most basic batching method is to directly concatenate the N data items and then add N to the front of the shape. In some scenarios, data from a certain period or several periods of time needs to be grouped into a batch and concatenated along a specific dimension. In such cases, the TimeBatch feature can be used to group the batch.

In the ASR\(Automatic Speech Recognition\) automatic speech recognition (ASR) scenario, there are two requirements: grouping a batch by a fixed time window, or grouping an entire batch by time segments (discontinuous time). Both can be implemented through TimeBatch.

## Usage

When constructing a DataFlow graph, users add the TimeBatch feature by setting attributes on the input of a FlowNode. The following is an example.

```cpp
TimeBatch time_batch = {};
// Set the value of each attribute in time_batch as required
time_batch.time_window = 10;
time_batch.batch_dim = 5;
time_batch.drop_remainder = true;
DataFlowInputAttr flow_attr = {DataFlowAttrType::TIME_BATCH, &time_batch};
std::vector<DataFlowInputAttr> flow_attrs = {flow_attr};
// Then set it through MapInput of the FlowNode
FlowNode::MapInput(xx, xx, xx, flow_attrs);
```

| Attribute Name | Data Type | Value Description |
| --- | --- | --- |
| time_window | int64_t | Integer (unit: ms). When the value is greater than 0, the batch is grouped by this time window. When the value is -1, the batch is grouped by time segments. Other values cause an error. |
| batch_dim | int64_t | This parameter takes effect only when time_window is set. Value range: [-1, shape dimension].<br><br>  - The default value -1 indicates that a batch dimension is added to dimension 0 of the output shape.<br>  - When shape dimension > batch_dim >= 0, the batch is grouped along a specific dimension.<br>  - When batch_dim > shape dimension or batch_dim < -1, an error is reported. |
| drop_remainder | bool | This parameter takes effect only when time_window is set.<br>It takes effect only when time_window > 0. It specifies whether to discard data when the duration is less than time_window. The default value is false, which means not to discard. true means to discard. Example:<br>If time_window = 5 ms and the input data duration is 3 ms:<br><br>  - When drop_remainder is not configured or is set to false, the input data is not discarded.<br>  - When drop_remainder is set to true, if the input data does not carry EOS or SEG, the system keeps waiting and does not discard the data. If the input data carries only SEG, the data is discarded. If the input data carries the EOS flag, the input data is discarded and only the EOS flag is passed.<br>  - If the input data does not carry EOS or SEG, the system keeps waiting and does not discard the data.<br>  - If the input data carries only SEG, the data is discarded.<br>  - If the input data carries the EOS flag, the input data is discarded and only the EOS flag is passed. |
| time_interval | int64_t | Not enabled |
| timeout | int64_t | Not enabled |
| flag | int32_t | Not enabled |
| padding | bool | Not enabled |

## Precautions

The current Batch feature cannot perform load balancing. Therefore, if a 2P environment is used, add \{"ge.exec.logicalDeviceClusterDeployMode", "SINGLE"\}, \{"ge.exec.logicalDeviceId", "\[0:0\]"\} during GE initialization. logicalDeviceId can be \[0:0\] or \[0:1\]. The details are as follows.

When logicalDeviceClusterDeployMode is SINGLE, it is used to specify that the model is deployed on a specific device.

Configuration format: \[node\_id:device\_id\]

- node\_id: logical ID of the Ascend AI Processor, starting from 0, indicating the device index in the resource configuration file.
- device\_id: physical ID of the Ascend AI Processor.
