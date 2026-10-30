# DataFlow API List

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T13:07:49.580Z pushedAt=2026-10-29T02:59:58.865Z nl=2 -->

Uses the DataFlow Python APIs to construct a DataFlow graph for inference. It supports defining graph processing nodes and UDF processing nodes, and describing the data flow relationships between processing nodes. It also supports importing IR files of TensorFlow, ONNX, and MindSpore as the computation logic definition of graph processing nodes.

## DataFlow Graph Construction APIs

**Table 1** DataFlow graph construction APIs

| Interface Name | Overview |
| --- | --- |
| [dataflow.FlowData](dataflow-FlowData.md) | A data node in a DataFlow Graph. Each FlowData corresponds to one input. |
| [FlowNode](FlowNode.md) | A computation node in a DataFlow Graph. |
| [add_process_point](add_process_point.md) | Adds a mapped pp to a FlowNode. Currently, a FlowNode can have only one pp. After the pp is added, the inputs and outputs of the FlowNode and the pp are mapped in order by default. |
| [map_input](map_input.md) | Maps the input of a FlowNode, indicating that the node_input_index-th input of the FlowNode is passed to the pp_input_index-th input of the ProcessPoint, and sets all attributes in attr for this input of the ProcessPoint. Returns the FlowNode node after mapping. This function is optional. If it is not called, the inputs of the FlowNode and the ProcessPoint are mapped in order by default. |
| [map_output](map_output.md) | Maps the output of a FlowNode, indicating that the pp_output_index-th output of the pp is passed to the node_output_index-th output of the FlowNode, and returns the FlowNode node after mapping. |
| [set_attr](set_attr.md) | Sets the attributes of a FlowNode. |
| [**call**](__call__.md) | Calls a FlowNode to perform computation. |
| [set_balance_scatter](set_balance_scatter.md) | Sets the balance scatter property of a node. A UDF with the balance scatter property can use balance options to set load-balanced output. |
| [set_balance_gather](set_balance_gather.md) | Sets the balance gather property of a node. A UDF with the balance gather property can use balance options to set load-balanced affinity output. |
| [set_alias](set_alias.md) | Sets the alias of a node. When option:ge.experiment.data_flow_deploy_info_path is used to specify the deployment location of a node, the flow_node_list field can use the alias for specification. |
| [dataflow.FlowFlag](dataflow-FlowFlag.md) | Sets the flags in the FlowMsg message header. |
| [FlowGraph](FlowGraph.md) | A DataFlow graph, which consists of input nodes (FlowData) and compute nodes (FlowNode). |
| [set_contains_n_mapping_node](set_contains_n_mapping_node.md) | Sets whether a FlowGraph contains an n_mapping node. |
| [set_inputs_align_attrs](set_inputs_align_attrs.md) | Sets the input alignment properties in a FlowGraph. |
| [set_exception_catch](set_exception_catch.md) | Sets whether the user exception catching function is enabled. |
| [dataflow.FlowOutput](dataflow-FlowOutput.md) | Describes the output of a FlowNode. |
| [dataflow.Framework](dataflow-Framework.md) | Sets the framework type of the original network model. |
| [FuncProcessPoint](FuncProcessPoint.md) | Constructor of FuncProcessPoint. Returns a FuncProcessPoint object. |
| [set_init_param](set_init_param.md) | Sets the initialization parameters of a FuncProcessPoint. |
| [add_invoked_closure](add_invoked_closure.md) | Adds a GraphProcessPoint or FlowGraphProcessPoint invoked by a FuncProcessPoint, and returns the FuncProcessPoint after the addition. |
| [GraphProcessPoint](GraphProcessPoint.md) | Constructor of GraphProcessPoint. Returns a GraphProcessPoint object. |
| [fnode](fnode.md) | Generates a FlowNode based on the current GraphProcessPoint, and returns a FlowNode object. |
| [dataflow.FlowGraphProcessPoint](dataflow-FlowGraphProcessPoint.md) | Constructor of GraphProcessPoint. Returns a GraphProcessPoint object. |
| [Tensor](Tensor.md) | Constructor of Tensor. |
| [numpy](numpy.md) | Converts a Tensor to a numpy ndarray. |
| [dataflow.TensorDesc](dataflow-TensorDesc.md) | Description function of Tensor. |
| [dataflow.alloc_tensor](dataflow-alloc_tensor.md) | Allocates a dataflow tensor based on the shape, data type, and alignment size. |
| [dataflow.utils.generate_deploy_template](dataflow-utils-generate_deploy_template.md) | Generates the template of the file required by option:"ge.experiment.data_flow_deploy_info_path" for the specified deployment location based on a FlowGraph. |
| [register](register.md) | Registers the serialization, deserialization, and size calculation functions corresponding to a custom type. It can be used together with the feed and fetch APIs to feed/fetch any Python type. |
| [registered](registered.md) | Determines whether a message type ID has been registered. |
| [get_msg_type(dataflow)](get_msg_type-dataflow.md) | Gets the registered message type ID based on the type definition. |
| [get_serialize_func](get_serialize_func.md) | Gets the registered serialization function based on the message type ID. |
| [get_deserialize_func](get_deserialize_func.md) | Gets the registered deserialization function based on the message type ID. |
| [get_size_func](get_size_func.md) | Gets the registered function for calculating the serialized memory size based on the message type ID. |
| [deserialize_from_file](deserialize_from_file.md) | Deserializes a serialized pickle file to restore a Python object. |
| [pyflow](pyflow.md) | Supports running a function as a pipeline task locally or remotely. |
| [method](method.md) | Supports running a class as a pipeline task locally or remotely in complex scenarios. |
| [npu_model](npu_model.md) | If a UDF is deployed on the host side, data needs to be copied from the device to the local side for computation during execution. In PyTorch scenarios, if all computation is on the device side and the inputs and outputs are also on the device side, data needs to be copied from the device to the host during execution, and PyTorch moves the data back to the device side after execution, which affects execution performance. Using npu_model can optimize this by triggering execution without moving data (that is, directly offloading execution to the device). |
| [dataflow.CountBatch](dataflow-CountBatch.md) | The CountBatch function groups multiple data items into a batch by batchSize based on a UDF as the computation processing node. This function applies to asynchronous dataflow scenarios. |
| [dataflow.TimeBatch](dataflow-TimeBatch.md) | The TimeBatch function is based on UDFs.<br>Normally, a model processes one data item at a time. When a batch of data needs to be processed at once, the data must be grouped into a batch. The most basic batching method is to directly concatenate the N data items and then add N to the front of the shape. In some scenarios, data of a certain period or several periods needs to be grouped into a batch and concatenated along a specific dimension. In such cases, the TimeBatch function can be used to group the batch. |

## DataFlow Runtime APIs

**Table 2** DataFlow runtime APIs

| Interface Name | Overview |
| --- | --- |
| [dataflow.init](dataflow-init.md) | Options for initializing dataflow. |
| [FlowInfo](FlowInfo.md) | Flow information of DataFlow. |
| [set_user_data](set_user_data.md) | Sets user information. |
| [get_user_data(dataflow)](get_user_data-dataflow.md) | Obtains user information. |
| [user_data](user_data.md) | Obtains user information. |
| [data_size](data_size.md) | Gets the length of user_data. |
| [start_time](start_time.md) | Reads and sets the start time of FlowInfo as an attribute. |
| [end_time](end_time.md) | Reads and sets the end time of FlowInfo as an attribute. |
| [flow_flags](flow_flags.md) | Reads and sets flow_flags of FlowInfo as an attribute. |
| [transaction_id](transaction_id.md) | Reads and writes the transaction ID as an attribute. |
| [feed_data](feed_data.md) | Feeds data into a Graph. |
| [feed](feed.md) | Feeds data into a Graph, supporting any serializable input. |
| [fetch_data](fetch_data.md) | Gets the output data of a Graph. |
| [fetch](fetch.md) | Gets the output data of a Graph, supporting any serializable output. |
| [dataflow.finalize](dataflow-finalize.md) | Releases the resources initialized by dataflow. |
| [dataflow.get_running_device_id](dataflow-get_running_device_id.md) | Gets the device_id on which the current UDF runs during UDF execution. The information source is the configuration of the UDF deployment location. |
| [dataflow.get_running_instance_id](dataflow-get_running_instance_id.md) | Gets the running instance ID of the current UDF during UDF execution. This information comes from the logic_device_list configuration in data_flow_deploy_info.json. |
| [dataflow.get_running_instance_num](dataflow-get_running_instance_num.md) | Gets the number of running instances of the current UDF during UDF execution. This information comes from the logic_device_list configuration in data_flow_deploy_info.json. |

## Module

dataflow module: namespace for public interfaces

## Classes

- class CountBatch: Class for CountBatch attributes
- class FlowData: Input node class
- class FlowFlag: Data flag class
- class FlowGraph: Graph class of dataflow
- class FlowInfo: Class for information carried by specified input and output data
- class FlowNode: Computation node class
- class FlowOutput: Output class of a computation node
- class Framework: Enumeration class of framework types for IR files
- class FuncProcessPoint: UDF processing node class
- class GraphProcessPoint: Graph Processing Node class
- class Tensor: Tensor data class
- class TensorDesc: Tensor description class
- class TimeBatch: TimeBatch attribute class

## Functions

- init\(...\): Resource initialization method of dataflow
- finalize\(...\): Resource release method of dataflow

## Other Members

**Table 3** Other Members

| Name | Overview |
| --- | --- |
| DT_FLOAT | Object of df.data_type.DType<br>32-bit single-precision floating-point number |
| DT_FLOAT16 | Object of df.data_type.DType<br>16-bit half-precision floating-point number |
| DT_INT8 | Object of df.data_type.DType<br>signed 8-bit integer |
| DT_INT16 | Object of df.data_type.DType<br>signed 16-bit integer |
| DT_UINT16 | Object of df.data_type.DType<br>unsigned 16-bit integer |
| DT_UINT8 | Object of df.data_type.DType<br>unsigned 8-bit integer |
| DT_INT32 | Object of df.data_type.DType<br>signed 32-bit integer |
| DT_INT64 | Object of df.data_type.DType<br>signed 64-bit integer |
| DT_UINT32 | Object of df.data_type.DType<br>unsigned 32-bit integer |
| DT_UINT64 | Object of df.data_type.DType<br>unsigned 64-bit integer |
| DT_BOOL | Object of df.data_type.DType<br>boolean type |
| DT_DOUBLE | Object of df.data_type.DType<br>64-bit double-precision floating-point number |
| DT_STRING | Object of df.data_type.DType<br>string type |
