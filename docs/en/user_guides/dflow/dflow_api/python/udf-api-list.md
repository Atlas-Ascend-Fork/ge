# UDF API List

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-29T07:38:30.931Z pushedAt=2026-10-29T07:53:28.363Z nl=2 -->

This document describes the external APIs provided by the UDF (User Defined Function) module. You can call these APIs to develop custom processing functions, and then execute the processing functions on the CPU through DataFlow graph construction.

You can view the implementation of the corresponding APIs in the “python/site-packages/dataflow/flow\_func/flow\_func.py” under the file storage path after CANN software installation. The API list is as follows.

## FlowMsg Class

Used for input and output related operations of FlowFunc.

**Table 1** FlowMsg class interface

| Interface Name | Overview |
| --- | --- |
| [FlowMsg Constructor](flowmsg-ctor.md) | Constructor of FlowMsg. |
| [get_msg_type (UDF)](get_msg_type-udf.md) | Obtains the message type of FlowMsg. |
| [get_tensor](get_tensor.md) | Obtains the tensor object in FlowMsg. |
| [set_ret_code](set_ret_code.md) | Sets the error code in the FlowMsg message. |
| [get_ret_code](get_ret_code.md) | Obtains the error code in the input FlowMsg message. |
| [set_start_time](set_start_time.md) | Sets the start timestamp in the FlowMsg message header. |
| [get_start_time](get_start_time.md) | Obtains the start timestamp in the FlowMsg message. |
| [set_end_time](set_end_time.md) | Sets the end timestamp in the FlowMsg message header. |
| [get_end_time](get_end_time.md) | Obtains the end timestamp in a FlowMsg message. |
| [set_flow_flags](set_flow_flags.md) | Sets the flags in the FlowMsg message header. |
| [get_flow_flags](get_flow_flags.md) | Obtains the flags in the FlowMsg message header. |
| [set_route_label](set_route_label.md) | Sets the route label. |
| [get_transaction_id](get_transaction_id.md) | Obtains the transaction ID in the FlowMsg message. The transaction ID starts from 1 and increments by 1 each time a batch of data is fed, which can be used to identify the batch of data. |
| [set_msg_type](set_msg_type.md) | Sets the message type of FlowMsg. |
| [get_raw_data](get_raw_data.md) | Obtains data of the raw data type. |
| [set_transaction_id](set_transaction_id.md) | Sets the transaction ID used for DataFlow data transmission. |

## Tensor Class

Used for operations related to Tensor. The Tensor obtained here is dataflow.Tensor.

**Table 2** Tensor class interface

| Interface Name | Overview |
| --- | --- |
| [Tensor Constructor](tensor-ctor.md) | Tensor constructor. |
| [get_shape](get_shape.md) | Obtains the Shape of the Tensor. |
| [get_data_type](get_data_type.md) | Obtains the data type in the Tensor. |
| [get_data_size](get_data_size.md) | Obtains the data size of a Tensor. |
| [get_element_cnt](get_element_cnt.md) | Obtains the number of elements in a Tensor. |
| [reshape](reshape.md) | Performs a reshape operation on a tensor without changing its content. |

## MetaParams Class

Used to obtain shared variable information through this class.

**Table 3** MetaParams class interface

| Interface Name | Overview |
| --- | --- |
| [PyMetaParams Constructor](pymetaparams-ctor.md) | PyMetaParams constructor. |
| [get_name](get_name.md) | Obtains the instance name of Flowfunc. |
| [get_attr_int](get_attr_int.md) | Obtains the int-type attribute value with the specified name. |
| [get_attr_bool_list](get_attr_bool_list.md) | Obtains the value of the bool array attribute with the specified name. |
| [get_attr_int_list](get_attr_int_list.md) | Obtains the value of the int array attribute with the specified name. |
| [get_attr_int_list_list](get_attr_int_list_list.md) | Obtains the attribute value of the int two-dimensional array type with the specified name. |
| [get_attr_bool](get_attr_bool.md) | Obtains the value of the bool-type attribute with the specified name. |
| [get_attr_float_list](get_attr_float_list.md) | Obtains the attribute value of the float array type with the specified name. |
| [get_attr_tensor_dtype](get_attr_tensor_dtype.md) | Obtains the attribute value of the numpy dtype type with the specified name. |
| [get_attr_tensor_dtype_list](get_attr_tensor_dtype_list.md) | Obtains the attribute value of the numpy dtype array type with the specified name. |
| [get_attr_str](get_attr_str.md) | Obtains the attribute value of the string type with the specified name. |
| [get_attr_str_list](get_attr_str_list.md) | Obtains the attribute value of the string array type with a specified name. |
| [get_attr_float](get_attr_float.md) | Obtains the value of the float-type attribute with the specified name. |
| [get_input_num](get_input_num.md) | Obtains the number of inputs of Flowfunc. |
| [get_output_num](get_output_num.md) | Obtains the number of outputs of Flowfunc. |
| [get_work_path](get_work_path.md) | Obtains the work path of Flowfunc. |
| [get_running_device_id](get_running_device_id.md) | Obtains the ID of the running device. |

## MetaRunContext Class

Used for context-related processing of the FlowFunc processing function, such as applying for tensors, setting outputs, and running FlowModel.

**Table 4** MetaRunContext class interface

| Interface Name | Overview |
| --- | --- |
| [MetaRunContext Constructor](metaruncontext-ctor.md) | MetaRunContext constructor. |
| [alloc_tensor_msg](alloc_tensor_msg.md) | Applies for a FlowMsg of the tensor type based on the shape, data type, and alignment size. |
| [set_output](set_output.md) | Sets the tensor of the output with the specified index. |
| [set_multi_outputs](set_multi_outputs.md) | Sets the tensor of the output with the specified index in batches. |
| [run_flow_model](run_flow_model.md) | Synchronously executes the specified model. |
| [alloc_empty_data_msg](alloc_empty_data_msg.md) | Allocates a message of the MsgType type for empty data. |
| [get_user_data (UDF)](get_user_data-udf.md) | Obtains user-defined data. |
| [raise_exception](raise_exception.md) | A UDF proactively reports an exception. |
| [get_exception](get_exception.md) | Obtains the exception reported by other UDF nodes. |
| [alloc_raw_data_msg](alloc_raw_data_msg.md) | Allocates a contiguous memory block based on the input size to carry FlowMsg of the raw data type. |
| [to_flow_msg](to_flow_msg.md) | Converts a dataflow Tensor into a FlowMsg. |

## AffinityPolicy Class

**Table 5** AffinityPolicy class APIs

| Interface Name | Overview |
| --- | --- |
| [AffinityPolicy Class](affinitypolicy-class.md) | Defines the affinity policy enumeration. |

## BalanceConfig Class

When balanced distribution is required, you need to set the output data identifier and weight matrix related configuration information. Based on the configuration, the scheduling module can complete balanced distribution among multiple instances.

**Table 6** BalanceConfig class APIs

| Interface Name | Overview |
| --- | --- |
| [BalanceConfig Constructor](balanceconfig-ctor.md) | Constructor of BalanceConfig. |
| [set_data_pos](set_data_pos.md) | Sets the position of the output data in the weight matrix. |
| [get_inner_config](get_inner_config.md) | Obtains the internal configuration object, which is called by [set_output](set_output.md) or [set_multi_outputs](set_multi_outputs.md). |

## FlowMsgQueue Class

In the streaming input scenario (that is, when the input parameter of the flow func function is a queue), it is used as the input queue of the flow func. After a FlowMsg in the queue is dequeued, it is converted into the corresponding data type based on MsgType and returned to the user.

**Table 7** FlowMsgQueue class APIs

| Interface Name | Overview |
| --- | --- |
| [FlowMsgQueue Constructor](flowmsgqueue-ctor.md) | FlowMsgQueue constructor and destructor. |
| [get](get.md) | Obtains an element from the queue. |
| [get_nowait](get_nowait.md) | Obtains an element from the queue without waiting, which is functionally equivalent to get(block=False). |
| [full](full.md) | Determines whether the queue is full. |
| [full](full.md) | Checks whether the queue is empty. |
| [qsize](qsize.md) | Obtains the number of elements currently in the queue. |

## UDF Log APIs

UDF Python provides log recording APIs. To use them, import the flow\_func module, use the logger object defined in it, and call the log APIs of different levels encapsulated by the logger object.

**Table 8** UDF log APIs

| Interface Name | Overview |
| --- | --- |
| [FlowFuncLogger Constructor](flowfunclogger-ctor.md) | Constructor of FlowFuncLogger. |
| [get_log_header](get_log_header.md) | Obtains the log extension header information. |
| [is_log_enable](is_log_enable.md) | Queries whether logs of the corresponding level and type are enabled. |
| [Error-Level Log Macro of Run Logs](run-log-error-level-log-macro.md) | Run log macro for the Error level. |
| [Info-Level Log Macro of Run Logs](run-log-info-level-log-macro.md) | Run log macro at the Info level. |
| [Error-Level Log Macro of Debug Logs](debug-log-error-level-log-macro.md) | Debug log macro for the Error level. |
| [Warn-Level Log Macro of Debug Logs](debug-log-warn-level-log-macro.md) | Debug log macro for Warn-level logs. |
| [Info-Level Log Macro of Debug Logs](debug-log-info-level-log-macro.md) | Debug log macro at the Info level. |
| [Debug-Level Log Macro of Debug Logs](debug-log-debug-level-log-macro.md) | Debug-level log macro. |
