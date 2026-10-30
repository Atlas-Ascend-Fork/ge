# UDF API List

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-28T12:57:48.380Z pushedAt=2026-10-29T02:47:34.684Z nl=2 -->

## AttrValue Class

**Table 2** AttrValue class APIs

| Interface Name | Overview |
| --- | --- |
| [AttrValue Constructor and Destructor](attrvalue-ctor-dtor.md) | Constructor and destructor of the AttrValue class. |
| [GetVal(AscendString &value)](getval-ascendstring-value.md) | Obtains the property value of the string type. |
| [GetVal(std::vector\<AscendString\> &value)](getval-std-vector-ascendstring-value.md) | Obtains the property value of the list string type. |
| [GetVal(int64_t &value)](getval-int64_t-value.md) | Obtains the property value of the int type. |
| [GetVal(std::vector<int64_t> &value)](getval-std-vector-int64_t-value.md) | Obtains the property value of the list int type. |
| [GetVal(std::vector<std::vector<int64_t >> &value)](getval-std-vector-std-vector-int64_t-value.md) | Obtains the property value of the list list int type. |
| [GetVal(float &value)](getval-float-value.md) | Obtains the property value of the float type. |
| [GetVal(std::vector\<float\> &value)](getval-std-vector-float-value.md) | Obtains the property value of the list float type. |
| [GetVal(bool &value)](getval-bool-value.md) | Obtains the property value of the bool type. |
| [GetVal(std::vector\<bool\> &value)](getval-std-vector-bool-value.md) | Obtains the property value of the list bool type. |
| [GetVal(TensorDataType &value)](getval-tensordatatype-value.md) | Obtains the property value of the TensorDataType type. |
| [GetVal(std::vector\<TensorDataType\> &value)](getval-std-vector-tensordatatype-value.md) | Obtains the property value of the list TensorDataType type. |

## AscendString Class

**Table 3** AscendString class APIs

| Interface Name | Overview |
| --- | --- |
| [AscendString Constructor and Destructor](ascendstring-ctor-dtor.md) | AscendString constructor and destructor. |
| [GetString](GetString.md) | Obtains the string address. |
| [Relational Operator Overloading](operator-overload.md) | For scenarios where the sizes of AscendString objects are compared (for example, sorting keys in a map data structure), this is implemented by overloading the following relational operators. |
| [GetLength](GetLength.md) | Obtains the length of the string. |

## MetaContext Class

**Table 4** MetaContext class APIs

| Interface Name | Overview |
| --- | --- |
| [MetaContext Constructor and Destructor](metacontext-ctor-dtor.md) | MetaContext constructor and destructor. |
| [AllocTensorMsg (MetaContext class)](alloctensormsg-metacontext-class.md) | Applies for a Tensor-type msg based on the shape and data type. This function is called by [Proc](Proc.md). |
| [AllocEmptyDataMsg (MetaContext class)](allocemptydatamsg-metacontext-class.md) | Applies for a message of the MsgType type with empty data. This function is called by [Proc](Proc.md). |
| [SetOutput (MetaContext class, tensor)](setoutput-metacontext-class-tensor.md) | Sets the tensor of the output at the specified index. This function is called by [Proc](Proc.md). |
| [GetAttr (MetaContext class, obtain pointer)](getattr-metacontext-class-get-ptr.md) | Obtains a pointer of the AttrValue type based on the attribute name. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetAttr (MetaContext class, obtain property value)](getattr-metacontext-class-get-attr-value.md) | Obtains the attribute value based on the attribute name. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [RunFlowModel (MetaContext class)](runflowmodel-metacontext-class.md) | Synchronously executes the specified model. This function is called by [Proc](Proc.md). |
| [GetInputNum (MetaContext class)](getinputnum-metacontext-class.md) | Obtains the number of inputs of Flowfunc. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetOutputNum (MetaContext class)](getoutputnum-metacontext-class.md) | Obtains the number of outputs of Flowfunc. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetWorkPath (MetaContext class)](getworkpath-metacontext-class.md) | Obtains the working path of Flowfunc. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetRunningDeviceId (MetaContext class)](getrunningdeviceid-metacontext-class.md) | Obtains the ID of the running device. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetUserData (MetaContext class)](getuserdata-metacontext-class.md) | Obtains user data. This function is called by [Proc](Proc.md). |
| [AllocTensorMsgWithAlign (MetaContext class)](alloctensormsgwithalign-metacontext-class.md) | Applies for a FlowMsg of the Tensor type based on the shape, data type, and alignment size. The difference from the [AllocTensorMsg](alloctensormsg-metacontext-class.md) function is that AllocTensorMsg applies for memory aligned to 64 bytes by default, whereas this function allows you to specify the alignment size for performance tuning. |
| [RaiseException (MetaContext class)](raiseexception-metacontext-class.md) | UDF proactively reports an exception, which can be caught by other UDFs in the same scope. |
| [GetException (MetaContext class)](getexception-metacontext-class.md) | A UDF obtains exceptions. If the exception capture function is enabled, the UDF must attempt to capture exceptions at the beginning of the Proc function. |

## FlowMsg Class

**Table 5** FlowMsg class APIs

| Interface Name | Overview |
| --- | --- |
| [FlowMsg Constructor and Destructor](flowmsg-ctor-dtor.md) | FlowMsg constructor and destructor. |
| [GetMsgType (FlowMsg Class)](getmsgtype-flowmsg-class.md) | Obtains the message type of FlowMsg. |
| [GetTensor (FlowMsg Class)](gettensor-flowmsg-class.md) | Obtains the Tensor pointer in FlowMsg. |
| [SetRetCode (FlowMsg Class)](setretcode-flowmsg-class.md) | Sets the error code in the FlowMsg message. |
| [GetRetCode (FlowMsg Class)](getretcode-flowmsg-class.md) | Obtains the error code in the input FlowMsg message. |
| [SetStartTime (FlowMsg Class)](setstarttime-flowmsg-class.md) | Sets the start timestamp in the FlowMsg message header. |
| [GetStartTime (FlowMsg Class)](getstarttime-flowmsg-class.md) | Obtains the start timestamp in the FlowMsg message. |
| [SetEndTime (FlowMsg Class)](setendtime-flowmsg-class.md) | Sets the end timestamp in the FlowMsg message header. |
| [GetEndTime (FlowMsg Class)](getendtime-flowmsg-class.md) | Obtains the end timestamp in a FlowMsg message. |
| [SetFlowFlags (FlowMsg Class)](setflowflags-flowmsg-class.md) | Sets the flags in the FlowMsg message header. |
| [GetFlowFlags (FlowMsg Class)](getflowflags-flowmsg-class.md) | Obtains the flags in the FlowMsg message header. |
| [SetRouteLabel](SetRouteLabel.md) | Sets the route label. |
| [GetTransactionId (FlowMsg Class)](gettransactionid-flowmsg-class.md) | Obtains the transaction ID in the FlowMsg message. The transaction ID starts from 1 and increments by one each time a batch of data is fed, which can be used to identify which batch of data it is. |
| [GetTensorList](GetTensorList.md) | Returns the list of all Tensor pointers in FlowMsg. |
| [GetRawData (FlowMsg Class)](getrawdata-flowmsg-class.md) | Obtains the data pointer and data size corresponding to the data of the RawData type. |
| [SetMsgType (FlowMsg Class)](setmsgtype-flowmsg-class.md) | Sets the message type of FlowMsg. |
| [SetTransactionId (FlowMsg Class)](settransactionid-flowmsg-class.md) | Sets the transaction ID in the FlowMsg message. |

## Tensor Class

**Table 6** Tensor class APIs

| Interface Name | Overview |
| --- | --- |
| [Tensor Constructor and Destructor](tensor-ctor-dtor.md) | Tensor constructor and destructor. |
| [GetShape](GetShape.md) | Obtains the shape of the Tensor. |
| [GetDataType](GetDataType.md) | Obtains the data type in the Tensor. |
| [GetData](GetData.md) | Obtains the data in a Tensor. |
| [GetDataSize](GetDataSize.md) | Obtains the data size of a Tensor. |
| [GetElementCnt](GetElementCnt.md) | Obtains the number of elements in a Tensor. |
| [GetDataBufferSize](GetDataBufferSize.md) | Obtains the aligned data size in the Tensor. |
| [Reshape](Reshape.md) | Performs a Reshape operation on the Tensor without changing its content. |

## MetaFlowFunc Class

**Table 7** MetaFlowFunc class APIs

| Interface Name | Overview |
| --- | --- |
| [MetaFlowFunc Constructor and Destructor](metaflowfunc-ctor-dtor.md) | Users inherit this class to implement a custom single-func handler function. In the destructor, release related resources. |
| [SetContext](SetContext.md) | Sets the context information of the flow func. |
| [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md) | User-defined initialization function of the flow func. |
| [Proc](Proc.md) | User-defined handler function of the flow func. |
| [RegisterFlowFunc](RegisterFlowFunc.md) | Registers the flow func.<br>Direct use of this function is not recommended. It is recommended to use the [MetaFlowFunc registration function macro](metaflowfunc-register-func-macro.md) to register the flow func. |
| [ResetFlowFuncState (MetaFlowFunc Class)](resetflowfuncstate-metaflowfunc-class.md) | Quickly resets the FlowFunc to the initialized state in fault recovery scenarios. |
| [Others](others.md) | REGISTER_FLOW_FUNC_INNER(name, ctr, clazz) and REGISTER_FLOW_FUNC_IMPL(name, ctr, clazz) are the implementations of the [MetaFlowFunc registration function macro](metaflowfunc-register-func-macro.md). Direct invocation by users is not recommended. |

## MetaMultiFunc Class

**Table 8** MetaMultiFunc Class Interfaces

| Interface Name | Overview |
| --- | --- |
| [MetaMultiFunc Constructor and Destructor](metamultifunc-ctor-dtor.md) | Users inherit this class to write custom multi-func handler functions. In the destructor, perform operations to release related resources. |
| [Init (MetaMultiFunc Class)](init-metamultifunc-class.md) | User-defined initialization function of flow func. |
| [Multi-Func Handler Function](multi-func-handler.md) | User-defined handler function for multiple flow funcs. |
| [RegisterMultiFunc](RegisterMultiFunc.md) | Registers multiple flow funcs.<br>Direct use of this function is not recommended. It is recommended to use the [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md) to register flow funcs. |
| [ResetFlowFuncState (MetaMultiFunc Class)](resetflowfuncstate-metamultifunc-class.md) | In fault recovery scenarios, quickly resets FlowFunc to the initialization state. |

## FlowFuncRegistrar Class

**Table 9** FlowFuncRegistrar Class Interfaces

| Interface Name | Overview |
| --- | --- |
| [RegProcFunc](RegProcFunc.md) | Registers the multi-flow-func handler function, combined with the [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md) to register flow funcs. |
| [CreateMultiFunc](CreateMultiFunc.md) | Creates a multi-func processing object and processing function. This API is used internally by the framework and is not directly used by users. |

## MetaParams Class

**Table 10** MetaParams class interface

| Interface Name | Overview |
| --- | --- |
| [MetaParams Constructor and Destructor](metaparams-ctor-dtor.md) | MetaParams constructor and destructor. |
| [GetName](GetName.md) | Obtains the instance name of Flowfunc. |
| [GetAttr (MetaParams class, get pointer)](getattr-metaparams-class-get-ptr.md) | Obtains a pointer of the AttrValue type based on the attribute name. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetAttr (MetaParams class, get property value)](getattr-metaparams-class-get-attr-value.md) | Obtains the attribute value based on the attribute name. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetInputNum (MetaParams class)](getinputnum-metaparams-class.md) | Obtains the number of inputs of Flowfunc. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetOutputNum (MetaParams class)](getoutputnum-metaparams-class.md) | Obtains the number of outputs of Flowfunc. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetWorkPath (MetaParams class)](getworkpath-metaparams-class.md) | Obtains the working path of Flowfunc. This function is called by [Init (MetaFlowFunc class)](init-metaflowfunc-class.md). |
| [GetRunningDeviceId (MetaParams class)](getrunningdeviceid-metaparams-class.md) | Obtains the ID of the running device. |

## MetaRunContext Class

**Table 11** MetaRunContext class interfaces

| Interface Name | Overview |
| --- | --- |
| [MetaRunContext Constructor and Destructor](metaruncontext-ctor-dtor.md) | MetaRunContext constructor and destructor. |
| [AllocTensorMsg (MetaRunContext class)](alloctensormsg-metaruncontext-class.md) | Applies for a Tensor-type msg based on the shape and data type. This function is called by [Proc](Proc.md). |
| [SetOutput (MetaRunContext class, tensor)](setoutput-metaruncontext-class-tensor.md) | Sets the tensor of the output at the specified index. This function is called by [Proc](Proc.md). |
| [RunFlowModel (MetaRunContext class)](runflowmodel-metaruncontext-class.md) | Synchronously executes the specified model. This function is called by [Proc](Proc.md). |
| [AllocEmptyDataMsg (MetaRunContext class)](allocemptydatamsg-metaruncontext-class.md) | Allocates a message of the MsgType type for empty data. |
| [GetUserData (MetaRunContext class)](getuserdata-metaruncontext-class.md) | Obtains user data. This function is called by [Proc](Proc.md). |
| [SetOutput (MetaRunContext class, output)](setoutput-metaruncontext-class-output.md) | Sets the output at the specified index and options. This function is called by the func function. |
| [SetMultiOutputs](SetMultiOutputs.md) | Sets outputs at the specified index and options in batches. This function is called by the func function. |
| [AllocTensorMsgWithAlign (MetaRunContext class)](alloctensormsgwithalign-metaruncontext-class.md) | Applies for a FlowMsg of the Tensor type based on the shape, data type, and alignment size. The difference from the [AllocTensorMsg](alloctensormsg-metacontext-class.md) function is that AllocTensorMsg applies for memory aligned to 64 bytes by default, whereas this function allows you to specify the alignment size for performance tuning. |
| [AllocTensorListMsg](AllocTensorListMsg.md) | Allocates a contiguous memory block based on the input dtype shapes array to hold a Tensor array. |
| [RaiseException (MetaRunContext class)](raiseexception-metaruncontext-class.md) | UDF proactively reports an exception, which can be caught by other UDFs in the same scope. |
| [GetException (MetaRunContext class)](getexception-metaruncontext-class.md) | A UDF obtains exceptions. If the exception capture function is enabled, the UDF must attempt to capture exceptions at the beginning of the Proc function. |
| [AllocRawDataMsg (MetaRunContext class)](allocrawdatamsg-metaruncontext-class.md) | Applies for a contiguous memory block of the specified size to store RawData type data. |
| [ToFlowMsg](ToFlowMsg.md) | Converts the input Tensor into a FlowMsg that carries the Tensor. |

## OutOptions Class

**Table 12** OutOptions class APIs

| Interface Name | Overview |
| --- | --- |
| [OutOptions Constructor and Destructor](outoptions-ctor-dtor.md) | Construction and destruction of OutOptions. |
| [MutableBalanceConfig](MutableBalanceConfig.md) | Obtains or creates the BalanceConfig. |
| [GetBalanceConfig](GetBalanceConfig.md) | Obtains the BalanceConfig. |

## BalanceConfig Class

**Table 13** BalanceConfig class APIs

| Interface Name | Overview |
| --- | --- |
| [BalanceConfig Constructor and Destructor](balanceconfig-ctor-dtor.md) | Constructor and destructor of BalanceConfig. |
| [SetAffinityPolicy](SetAffinityPolicy.md) | Sets the balanced distribution affinity. |
| [GetAffinityPolicy](GetAffinityPolicy.md) | Obtains the affinity. |
| [SetBalanceWeight](SetBalanceWeight.md) | Sets the balanced distribution weight information. |
| [GetBalanceWeight](GetBalanceWeight.md) | Obtains the balanced distribution weight information. |
| [SetDataPos](SetDataPos.md) | Sets the position of the output data in the weight matrix. |
| [GetDataPos](GetDataPos.md) | Obtains the position of the output data in the weight matrix. |

## FlowBufferFactory

**Table 14** FlowBufferFactory class APIs

| Interface Name | Overview |
| --- | --- |
| [AllocTensor (FlowBufferFactory)](alloctensor-flowbufferfactory-class.md) | AllocTensor allocates a Tensor based on the shape, data type, and alignment size. By default, the memory is aligned to 64 bytes. You can specify the alignment size to facilitate performance tuning. |

## FlowMsgQueue Class

**Table 15** FlowMsgQueue class APIs

| Interface Name | Overview |
| --- | --- |
| [FlowMsgQueue Constructor and Destructor](flowmsgqueue-ctor-dtor.md) | Constructor and destructor of FlowMsgQueue. |
| [Dequeue](Dequeue.md) | Sets the balanced distribution affinity. |
| [Depth](Depth.md) | Obtains the depth of the queue, that is, the maximum number of elements that the queue can hold. |
| [Size](Size.md) | Obtains the number of elements currently in the queue. |

## Registration Macro

**Table 16** Registration Macro

| Interface Name | Overview |
| --- | --- |
| [MetaFlowFunc Registration Function Macro](metaflowfunc-register-func-macro.md) | Registers the implementation class of MetaFlowFunc. |
| [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md) | Registers the implementation class of MetaMultiFunc. |

## UDF Log APIs

**Table 17** UDF log APIs

| Interface Name | Overview |
| --- | --- |
| [FlowFuncLogger Constructor and Destructor](flowfunclogger-ctor-dtor.md) | The constructor and destructor of FlowFuncLogger. |
| [GetLogger](GetLogger.md) | Obtains the log implementation class. |
| [GetLogExtHeader](GetLogExtHeader.md) | Obtains the log extension header information. |
| [IsLogEnable](IsLogEnable.md) | Queries whether logging of the corresponding level and type is enabled. |
| [Error](Error.md) | Records ERROR-level logs. |
| [Warn](Warn.md) | Records a log at the Warn level. |
| [Info](Info.md) | Records a log at the Info level. |
| [Debug](Debug.md) | Records Debug-level logs. |
| [Error-Level Log Macro of Run Logs](run-log-error-level-log-macro.md) | Runtime log macro for the Error level. |
| [Info-Level Log Macro of Debug Logs](run-log-info-level-log-macro.md) | Runtime log macro for the Info level. |
| [Error-Level Log Macro of Debug Logs](debug-log-error-level-log-macro.md) | Debug log macro for the Error level. |
| [Warn-Level Log Macro of Debug Logs](debug-log-warn-level-log-macro.md) | Debug log macro for Warn-level logs. |
| [Info-Level Log Macro of Debug Logs](debug-log-info-level-log-macro.md) | Debug log macro at the Info level. |
| [Debug-Level Log Macro of Debug Logs](debug-log-debug-level-log-macro.md) | Debug-level log macro. |

## Error Code

**Table 18** Error Code

| Error Code Module | Overview |
| --- | --- |
| [flowfunc](udf-error-code.md#flowfunc) | Provides the flowfunc error codes for users, mainly for determining and handling abnormal logic. |
| [AICPU](udf-error-code.md#aicpu) | Error codes that AICPU may report to users during model execution. |
