# DataFlow API List

<!-- md-trans-meta sourceCommit=2bcdf9e6f737036192b9d97b6ce81ad24f919f18 translatedAt=2026-11-16T05:14:22.960Z pushedAt=2026-11-16T07:47:38.250Z nl=2 -->

Uses the DataFlow Python API to construct a DataFlow graph for inference. It supports defining graph processing points and UDF processing points, and describing the data flow relationships between processing points. It also supports importing IR files of TensorFlow, ONNX, and MindSpore as the compute logic definition of graph processing points.

## DataFlow Graph Construction APIs

**Table 1** DataFlow graph construction APIs

| Interface Name | Overview |
| --- | --- |
| [dataflow.FlowData](dataflow-FlowData.md) | A data node in a DataFlow Graph. Each FlowData corresponds to one input. |
| [FlowNode](FlowNode.md) | A compute node in a DataFlow Graph. |
| [add_process_point](add_process_point.md) | Adds a mapped pp to a FlowNode. Currently, a FlowNode can add only one pp. After the pp is added, the inputs and outputs of the FlowNode and the pp are mapped in order by default. |
| [map_input](map_input.md) | Maps the input of a FlowNode, indicating that the node_input_index-th input of the FlowNode is passed to the pp_input_index-th input of the ProcessPoint, and sets all attributes in attr for this input of the ProcessPoint. This function is optional. If it is not called, the inputs of the FlowNode and the ProcessPoint are mapped in order by default. |
| [map_output](map_output.md) | Maps the output of a FlowNode, indicating that the pp_output_index-th output of the pp is passed to the node_output_index-th output of the FlowNode. |
| [set_attr](set_attr.md) | Sets the attributes of a FlowNode. |
| [**call**](__call__.md) | Calls a FlowNode to perform computation. |
| [set_balance_scatter](set_balance_scatter.md) | Sets the balance scatter property of a node. A UDF with the balance scatter property can use balance options to set load-balanced output. |
| [set_balance_gather](set_balance_gather.md) | Sets the balance gather property of a node. A UDF with the balance gather property can use balance options to set load-balanced affinity output. |
| [set_alias](set_alias.md) | Sets the node alias. When option:ge.experiment.data_flow_deploy_info_path is used to specify the node deployment location, the flow_node_list field can be specified using the alias. |
| [dataflow.FlowFlag](dataflow-FlowFlag.md) | Sets the flags in the FlowMsg message header. |
| [FlowGraph](FlowGraph.md) | A DataFlow graph, which consists of input nodes (FlowData) and compute nodes (FlowNode). |
| [set_contains_n_mapping_node](set_contains_n_mapping_node.md) | Sets whether a FlowGraph contains an n_mapping node. |
| [set_inputs_align_attrs](set_inputs_align_attrs.md) | Sets the input alignment properties in a FlowGraph. |
| [set_exception_catch](set_exception_catch.md) | Sets whether the user exception catching function is enabled. |
| [dataflow.FlowOutput](dataflow-FlowOutput.md) | Describes the output of a FlowNode. |
| [dataflow.Framework](dataflow-Framework.md) | Sets the framework type of the original network model. |
| [FuncProcessPoint](FuncProcessPoint.md) | Constructor of FuncProcessPoint. It returns a FuncProcessPoint object. |
| [set_init_param](set_init_param.md) | Sets the initialization parameters of FuncProcessPoint. |
| [add_invoked_closure](add_invoked_closure.md) | Adds a GraphProcessPoint or FlowGraphProcessPoint invoked by FuncProcessPoint. |
| [GraphProcessPoint](GraphProcessPoint.md) | GraphProcessPoint constructor, which returns a GraphProcessPoint object. |
| [fnode](fnode.md) | Generates a FlowNode based on the current GraphProcessPoint and returns a FlowNode object. |
| [dataflow.FlowGraphProcessPoint](dataflow-FlowGraphProcessPoint.md) | GraphProcessPoint constructor, which returns a GraphProcessPoint object. |
| [Tensor](Tensor.md) | Constructor of Tensor. |
| [numpy](numpy.md) | Converts a Tensor to a numpy ndarray. |
| [dataflow.TensorDesc](dataflow-TensorDesc.md) | Describes a Tensor. |
| [dataflow.alloc_tensor](dataflow-alloc_tensor.md) | Allocates a dataflow tensor based on the shape, data type, and alignment size. |
| [dataflow.utils.generate_deploy_template](dataflow-utils-generate_deploy_template.md) | Generates a template of the file required by option:"ge.experiment.data_flow_deploy_info_path" at the specified deployment location based on the FlowGraph. |
| [register](register.md) | Registers the functions for serialization, deserialization, and size calculation of a custom type. It can be used together with the feed and fetch APIs to feed/fetch any Python type. |
| [registered](registered.md) | Checks whether a message type ID has been registered. |
| [get_msg_type(dataflow)](get_msg_type-dataflow.md) | Obtains the registered message type ID based on the type definition. |
| [get_serialize_func](get_serialize_func.md) | Obtains the registered serialization function based on the message type ID. |
| [get_deserialize_func](get_deserialize_func.md) | Obtains the registered deserialization function based on the message type ID. |
| [get_size_func](get_size_func.md) | Obtains the registered function for computing the serialized memory size based on the message type ID. |
| [deserialize_from_file](deserialize_from_file.md) | Deserializes a serialized pickle file to restore a Python object. |
| [pyflow](pyflow.md) | Supports running a function as a pipeline task locally or remotely. |
| [method](method.md) | For complex scenarios, supports running a class as a pipeline task locally or remotely. |
| [npu_model](npu_model.md) | If a UDF is deployed on the host side, data needs to be copied from the device to the local memory for computation during execution. In the PyTorch scenario, if all computation is performed on the device side and the input and output are also on the device side, data needs to be copied from the device to the host during execution, and PyTorch then moves the data back to the device after execution, which affects execution performance. Using npu_model can optimize this by triggering execution in a way that does not move data (that is, directly offloading execution to the device). |
| [dataflow.CountBatch](dataflow-CountBatch.md) | The CountBatch feature groups multiple data items into a batch based on batchSize for UDF-based computation processing points. This feature applies to dataflow asynchronous scenarios. |
| [dataflow.TimeBatch](dataflow-TimeBatch.md) | The TimeBatch feature is based on UDF.<br>A normal model processes one data item at a time. When a batch of data needs to be processed at once, the data needs to be grouped into a batch. The most basic batching method is to directly concatenate the N data items and then add N to the front of the shape. In some scenarios, data of a certain period or several periods needs to be grouped into a batch and concatenated along a specific dimension. In this case, the TimeBatch feature can be used to group the batch. |

## DataFlow Runtime APIs

**Table 2** DataFlow runtime APIs

| Interface Name | Overview |
| --- | --- |
| [dataflow.init](dataflow-init.md) | Options for initializing dataflow. |
| [FlowInfo](FlowInfo.md) | Flow information of DataFlow. |
| [set_user_data](set_user_data.md) | Sets user information. |
| [get_user_data(dataflow)](get_user_data-dataflow.md) | Obtains user information. |
| [user_data](user_data.md) | Obtains user information. |
| [data_size](data_size.md) | Obtains the length of user_data. |
| [start_time](start_time.md) | Reads and sets the start time of FlowInfo as a property. |
| [end_time](end_time.md) | Reads and sets the end time of FlowInfo using the property method. |
| [flow_flags](flow_flags.md) | Reads and sets the flow_flags of FlowInfo through the property method. |
| [transaction_id](transaction_id.md) | Reads and writes the transaction ID as an attribute. |
| [feed_data](feed_data.md) | Feeds data into a Graph. |
| [feed](feed.md) | Feeds data into a Graph, supporting any serializable input. |
| [fetch_data](fetch_data.md) | Obtains the Graph output data. |
| [fetch](fetch.md) | Obtains the output data of a Graph. Any serializable output is supported. |
| [dataflow.finalize](dataflow-finalize.md) | Releases the resources initialized by dataflow. |
| [dataflow.get_running_device_id](dataflow-get_running_device_id.md) | Obtains the running device_id of the current UDF during UDF execution. The information comes from the configuration of the UDF deployment location. |
| [dataflow.get_running_instance_id](dataflow-get_running_instance_id.md) | Obtains the running instance ID of the current UDF during UDF execution. The information comes from the logic_device_list configuration in data_flow_deploy_info.json. |
| [dataflow.get_running_instance_num](dataflow-get_running_instance_num.md) | Obtains the number of running instances of the current UDF during UDF execution. The information comes from the logic_device_list configuration in data_flow_deploy_info.json. |

## Modules

dataflow module: namespace of public APIs

## Classes

- class CountBatch: attribute class of CountBatch
- class FlowData: input node class
- class FlowFlag: data flag class
- class FlowGraph: graph class of dataflow
- class FlowInfo: class that specifies the information carried by input and output data
- class FlowNode: compute node class
- class FlowOutput: output class of a compute node
- class Framework: enumeration class of the framework type of IR files
- class FuncProcessPoint: UDF processing point class
- class GraphProcessPoint: graph processing point class
- class Tensor: tensor data class
- class TensorDesc: description class of a tensor
- class TimeBatch: attribute class of TimeBatch

## Functions

- init\(...\): resource initialization method of dataflow
- finalize\(...\): resource release method of dataflow

## Other Members

**Table 3** Other members

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
