# UDF API List

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T04:14:35.564Z pushedAt=2026-11-16T06:24:00.110Z nl=2 -->

## AttrValue Class

**Table 2** AttrValue class APIs

| Interface Name | Overview |
| --- | --- |
| [AttrValue Constructor and Destructor](attrvalue-ctor-dtor.md) | AttrValue constructor and destructor. |
| [GetVal(AscendString &value)](getval-ascendstring-value.md) | Gets the attribute value of the string type. |
| [GetVal(std::vector\<AscendString\> &value)](getval-std-vector-ascendstring-value.md) | Gets the attribute value of the list string type. |
| [GetVal(int64_t &value)](getval-int64_t-value.md) | Gets the attribute value of the int type. |
| [GetVal(std::vector<int64_t> &value)](getval-std-vector-int64_t-value.md) | Gets the attribute value of the list int type. |
| [GetVal(std::vector<std::vector<int64_t >> &value)](getval-std-vector-std-vector-int64_t-value.md) | Gets the attribute value of the list list int type. |
| [GetVal(float &value)](getval-float-value.md) | Gets the attribute value of the float type. |
| [GetVal(std::vector\<float\> &value)](getval-std-vector-float-value.md) | Gets the attribute value of the list float type. |
| [GetVal(bool &value)](getval-bool-value.md) | Gets the attribute value of the bool type. |
| [GetVal(std::vector\<bool\> &value)](getval-std-vector-bool-value.md) | Gets the attribute value of the list bool type. |
| [GetVal(TensorDataType &value)](getval-tensordatatype-value.md) | Gets the attribute value of the TensorDataType type. |
| [GetVal(std::vector\<TensorDataType\> &value)](getval-std-vector-tensordatatype-value.md) | Gets the attribute value of the list TensorDataType type. |

## AscendString Class

**Table 3** AscendString class APIs

| Interface Name | Overview |
| --- | --- |
| [AscendString Constructor and Destructor](ascendstring-ctor-dtor.md) | AscendString constructor and destructor. |
| [GetString](GetString.md) | Obtains the string address. |
| [Relational Operator Overloading](operator-overload.md) | For scenarios where the sizes of AscendString objects are compared (for example, sorting keys of a map data structure), this is implemented by overloading the following relational operators. |
| [GetLength](GetLength.md) | Obtains the length of a string. |

## MetaContext Class

**Table 4** MetaContext class APIs

| Interface Name | Overview |
| --- | --- |
| [MetaContext Constructor and Destructor](metacontext-ctor-dtor.md) | MetaContext constructor and destructor. |
| [AllocTensorMsg (MetaContext Class)](alloctensormsg-metacontext-class.md) | Requests a Tensor-type msg based on the shape and data type. This function is called by [Proc](Proc.md). |
| [AllocEmptyDataMsg (MetaContext Class)](allocemptydatamsg-metacontext-class.md) | Requests a message of the MsgType type for empty activation. This function is called by [Proc](Proc.md). |
| [SetOutput (MetaContext Class, Tensor)](setoutput-metacontext-class-tensor.md) | Sets the tensor of the output at the specified index. This function is called by [Proc](Proc.md). |
| [GetAttr (MetaContext Class, Obtaining a Pointer)](getattr-metacontext-class-get-ptr.md) | Obtains a pointer of the AttrValue type based on the attribute name. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetAttr (MetaContext Class, Obtaining Attribute Values)](getattr-metacontext-class-get-attr-value.md) | Obtains the corresponding attribute value based on the attribute name. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [RunFlowModel (MetaContext Class)](runflowmodel-metacontext-class.md) | Synchronously executes the specified model. This function is called by [Proc](Proc.md). |
| [GetInputNum (MetaContext Class)](getinputnum-metacontext-class.md) | Obtains the number of inputs of Flowfunc. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetOutputNum (MetaContext Class)](getoutputnum-metacontext-class.md) | Obtains the number of outputs of Flowfunc. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetWorkPath (MetaContext Class)](getworkpath-metacontext-class.md) | Gets the working path of Flowfunc. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetRunningDeviceId (MetaContext Class)](getrunningdeviceid-metacontext-class.md) | Obtains the ID of the running device. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetUserData (MetaContext Class)](getuserdata-metacontext-class.md) | Obtains the user activation. This function is called by [Proc](Proc.md). |
| [AllocTensorMsgWithAlign (MetaContext Class)](alloctensormsgwithalign-metacontext-class.md) | Applies for a FlowMsg of the Tensor type based on the shape, data type, and alignment size. The difference from the [AllocTensorMsg](alloctensormsg-metacontext-class.md) function is that AllocTensorMsg applies for memory aligned to 64 bytes by default, whereas this function allows the alignment size to be specified for performance tuning. |
| [RaiseException (MetaContext Class)](raiseexception-metacontext-class.md) | A UDF actively reports an exception, which can be caught by other UDFs in the same scope. |
| [GetException (MetaContext Class)](getexception-metacontext-class.md) | UDF obtains an exception. If the exception catching feature is enabled, an attempt to catch an exception is required at the beginning of the Proc function in the UDF. |

## FlowMsg Class

**Table 5** FlowMsg class APIs

| Interface Name | Overview |
| --- | --- |
| [FlowMsg Constructor and Destructor](flowmsg-ctor-dtor.md) | FlowMsg constructor and destructor. |
| [GetMsgType (FlowMsg Class)](getmsgtype-flowmsg-class.md) | Obtains the message type of FlowMsg. |
| [GetTensor (FlowMsg Class)](gettensor-flowmsg-class.md) | Gets the Tensor pointer in FlowMsg. |
| [SetRetCode (FlowMsg Class)](setretcode-flowmsg-class.md) | Sets the error code in the FlowMsg message. |
| [GetRetCode (FlowMsg Class)](getretcode-flowmsg-class.md) | Obtains the error code in the input FlowMsg message. |
| [SetStartTime (FlowMsg Class)](setstarttime-flowmsg-class.md) | Sets the start timestamp in the FlowMsg message header. |
| [GetStartTime (FlowMsg Class)](getstarttime-flowmsg-class.md) | Obtains the start timestamp in the FlowMsg message. |
| [SetEndTime (FlowMsg Class)](setendtime-flowmsg-class.md) | Sets the end timestamp in the FlowMsg message header. |
| [GetEndTime (FlowMsg Class)](getendtime-flowmsg-class.md) | Obtains the end timestamp in a FlowMsg message. |
| [SetFlowFlags (FlowMsg Class)](setflowflags-flowmsg-class.md) | Sets the flags in the FlowMsg message header. |
| [GetFlowFlags (FlowMsg Class)](getflowflags-flowmsg-class.md) | Obtains the flags in the FlowMsg message header. |
| [SetRouteLabel](SetRouteLabel.md) | Sets the route label. |
| [GetTransactionId (FlowMsg Class)](gettransactionid-flowmsg-class.md) | Obtains the transaction ID in a FlowMsg message. The transaction ID starts from 1 and increments by 1 each time a batch of data is fed, which can be used to identify the batch of data. |
| [GetTensorList](GetTensorList.md) | Returns the pointer list of all Tensors in FlowMsg. |
| [GetRawData (FlowMsg Class)](getrawdata-flowmsg-class.md) | Gets the data pointer and data size corresponding to data of the RawData type. |
| [SetMsgType (FlowMsg Class)](setmsgtype-flowmsg-class.md) | Sets the message type of FlowMsg. |
| [SetTransactionId (FlowMsg Class)](settransactionid-flowmsg-class.md) | Sets the transaction ID in the FlowMsg message. |

## Tensor Class

**Table 6** Tensor class APIs

| Interface Name | Overview |
| --- | --- |
| [Tensor Constructor and Destructor](tensor-ctor-dtor.md) | Tensor constructor and destructor. |
| [GetShape](GetShape.md) | Obtains the Shape of the Tensor. |
| [GetDataType](GetDataType.md) | Obtains the data type in the Tensor. |
| [GetData](GetData.md) | Obtains the data in the Tensor. |
| [GetDataSize](GetDataSize.md) | Obtains the data size of a Tensor. |
| [GetElementCnt](GetElementCnt.md) | Obtains the number of elements in a Tensor. |
| [GetDataBufferSize](GetDataBufferSize.md) | Obtains the aligned data size in a Tensor. |
| [Reshape](Reshape.md) | Performs a reshape operation on the Tensor without changing its content. |

## MetaFlowFunc Class

**Table 7** MetaFlowFunc class APIs

| Interface Name | Overview |
| --- | --- |
| [MetaFlowFunc Constructor and Destructor](metaflowfunc-ctor-dtor.md) | The user inherits this class for writing a custom single-func handler function. In the destructor, operations to release related resources are performed. |
| [SetContext](SetContext.md) | Sets the context information of the flow func. |
| [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md) | The initialization function of the user-defined flow func. |
| [Proc](Proc.md) | The handler function of the user-defined flow func. |
| [RegisterFlowFunc](RegisterFlowFunc.md) | Registers a flow func.<br>It is not recommended to use this function directly. You are advised to use the [MetaFlowFunc Registration Function Macro](metaflowfunc-register-func-macro.md) to register a flow func. |
| [ResetFlowFuncState (MetaFlowFunc Class)](resetflowfuncstate-metaflowfunc-class.md) | In fault recovery scenarios, quickly resets FlowFunc to the initialization state. |
| [Others](others.md) | REGISTER_FLOW_FUNC_INNER(name, ctr, clazz) and REGISTER_FLOW_FUNC_IMPL(name, ctr, clazz) are the implementations of the [MetaFlowFunc Registration Function Macro](metaflowfunc-register-func-macro.md). It is not recommended that users call them directly. |

## MetaMultiFunc Class

**Table 8** MetaMultiFunc class APIs

| Interface Name | Overview |
| --- | --- |
| [MetaMultiFunc Constructor and Destructor](metamultifunc-ctor-dtor.md) | The user inherits this class for writing a custom multi-func handler function. In the destructor, operations to release related resources are performed. |
| [Init (MetaMultiFunc Class)](init-metamultifunc-class.md) | The initialization function of the user-defined flow func. |
| [Multi-Func Processing Function](multi-func-handler.md) | A user-defined multi-flow func handler function. |
| [RegisterMultiFunc](RegisterMultiFunc.md) | Registers multiple flow funcs.<br>It is not recommended to use this function directly. You are advised to use the [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md) to register a flow func. |
| [ResetFlowFuncState (MetaMultiFunc Class)](resetflowfuncstate-metamultifunc-class.md) | In fault recovery scenarios, quickly resets FlowFunc to the initialization state. |

## FlowFuncRegistrar Class

**Table 9** FlowFuncRegistrar class APIs

| Interface Name | Overview |
| --- | --- |
| [RegProcFunc](RegProcFunc.md) | Registers a multi-flow func handler function, and registers a flow func in combination with the [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md). |
| [CreateMultiFunc](CreateMultiFunc.md) | Creates a multi-func processing object and handler function. This function is used internally by the framework and is not directly used by users. |

## MetaParams Class

**Table 10** MetaParams class APIs

| Interface Name | Overview |
| --- | --- |
| [MetaParams Constructor and Destructor](metaparams-ctor-dtor.md) | MetaParams constructor and destructor. |
| [GetName](GetName.md) | Obtains the instance name of Flowfunc. |
| [GetAttr (MetaParams Class, Obtaining a Pointer)](getattr-metaparams-class-get-ptr.md) | Obtains a pointer of the AttrValue type based on the attribute name. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetAttr (MetaParams Class, Obtaining Attribute Values)](getattr-metaparams-class-get-attr-value.md) | Obtains the corresponding attribute value based on the attribute name. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetInputNum (MetaParams Class)](getinputnum-metaparams-class.md) | Obtains the number of inputs of Flowfunc. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetOutputNum (MetaParams Class)](getoutputnum-metaparams-class.md) | Obtains the number of outputs of Flowfunc. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetWorkPath (MetaParams Class)](getworkpath-metaparams-class.md) | Gets the working path of Flowfunc. This function is called by [Init (MetaFlowFunc Class)](init-metaflowfunc-class.md). |
| [GetRunningDeviceId (MetaParams Class)](getrunningdeviceid-metaparams-class.md) | Obtains the ID of the running device. |

## MetaRunContext Class

**Table 11** MetaRunContext class APIs

| Interface Name | Overview |
| --- | --- |
| [MetaRunContext Constructor and Destructor](metaruncontext-ctor-dtor.md) | MetaRunContext constructor and destructor. |
| [AllocTensorMsg (MetaRunContext Class)](alloctensormsg-metaruncontext-class.md) | Requests a Tensor-type msg based on the shape and data type. This function is called by [Proc](Proc.md). |
| [SetOutput (MetaRunContext Class, Tensor)](setoutput-metaruncontext-class-tensor.md) | Sets the tensor of the output at the specified index. This function is called by [Proc](Proc.md). |
| [RunFlowModel (MetaRunContext Class)](runflowmodel-metaruncontext-class.md) | Synchronously executes the specified model. This function is called by [Proc](Proc.md). |
| [AllocEmptyDataMsg (MetaRunContext Class)](allocemptydatamsg-metaruncontext-class.md) | Allocates a message of the MsgType type for empty data. |
| [GetUserData (MetaRunContext Class)](getuserdata-metaruncontext-class.md) | Obtains the user activation. This function is called by [Proc](Proc.md). |
| [SetOutput (MetaRunContext Class, Output)](setoutput-metaruncontext-class-output.md) | Sets the output at the specified index and options. This function is called by the func function. |
| [SetMultiOutputs](SetMultiOutputs.md) | Sets the outputs at the specified index and options in batches. This function is called by the func function. |
| [AllocTensorMsgWithAlign (MetaRunContext Class)](alloctensormsgwithalign-metaruncontext-class.md) | Applies for a FlowMsg of the Tensor type based on the shape, data type, and alignment size. The difference from the [AllocTensorMsg](alloctensormsg-metacontext-class.md) function is that AllocTensorMsg applies for memory aligned to 64 bytes by default, whereas this function allows the alignment size to be specified for performance tuning. |
| [AllocTensorListMsg](AllocTensorListMsg.md) | Allocates a contiguous buffer based on the input dtype shapes array to hold a Tensor array. |
| [RaiseException (MetaRunContext Class)](raiseexception-metaruncontext-class.md) | A UDF actively reports an exception, which can be caught by other UDFs in the same scope. |
| [GetException (MetaRunContext Class)](getexception-metaruncontext-class.md) | UDF obtains an exception. If the exception catching feature is enabled, an attempt to catch an exception is required at the beginning of the Proc function in the UDF. |
| [AllocRawDataMsg (MetaRunContext Class)](allocrawdatamsg-metaruncontext-class.md) | Requests a contiguous buffer based on the input size to carry RawData type data. |
| [ToFlowMsg](ToFlowMsg.md) | Converts the input Tensor into a FlowMsg that carries the Tensor. |

## OutOptions Class

**Table 12** OutOptions class APIs

| Interface Name | Overview |
| --- | --- |
| [OutOptions Constructor and Destructor](outoptions-ctor-dtor.md) | OutOptions constructor and destructor. |
| [MutableBalanceConfig](MutableBalanceConfig.md) | Gets or creates the BalanceConfig. |
| [GetBalanceConfig](GetBalanceConfig.md) | Obtains the BalanceConfig. |

## BalanceConfig Class

**Table 13** BalanceConfig class APIs

| Interface Name | Overview |
| --- | --- |
| [BalanceConfig Constructor and Destructor](balanceconfig-ctor-dtor.md) | Constructor and destructor of BalanceConfig. |
| [SetAffinityPolicy](SetAffinityPolicy.md) | Sets the balanced distribution affinity. |
| [GetAffinityPolicy](GetAffinityPolicy.md) | Obtains the affinity. |
| [SetBalanceWeight](SetBalanceWeight.md) | Sets balanced distribution weight information. |
| [GetBalanceWeight](GetBalanceWeight.md) | Obtains balanced distribution weight information. |
| [SetDataPos](SetDataPos.md) | Sets the position of the output data in the weight matrix. |
| [GetDataPos](GetDataPos.md) | Obtains the position of the output activation in the weight matrix. |

## FlowBufferFactory Class

**Table 14** FlowBufferFactory class APIs

| Interface Name | Overview |
| --- | --- |
| [AllocTensor (FlowBufferFactory Class)](alloctensor-flowbufferfactory-class.md) | Allocates a Tensor according to the shape, data type, and alignment size. By default, the memory is aligned to 64 bytes. You can specify the alignment size to facilitate performance tuning. |

## FlowMsgQueue Class

**Table 15** FlowMsgQueue class APIs

| Interface Name | Overview |
| --- | --- |
| [FlowMsgQueue Constructor and Destructor](flowmsgqueue-ctor-dtor.md) | FlowMsgQueue constructor and destructor. |
| [Dequeue](Dequeue.md) | Sets the balanced distribution affinity. |
| [Depth](Depth.md) | Obtains the depth of a queue, that is, the maximum number of elements that the queue can hold. |
| [Size](Size.md) | Obtains the number of elements currently in the queue. |

## Registration Macros

**Table 16** Registration macros

| Interface Name | Overview |
| --- | --- |
| [MetaFlowFunc Registration Function Macro](metaflowfunc-register-func-macro.md) | Registers the implementation class of MetaFlowFunc. |
| [MetaMultiFunc Registration Function Macro](metamultifunc-register-func-macro.md) | Registers the implementation class of MetaMultiFunc. |

## UDF Log APIs

**Table 17** UDF log APIs

| Interface Name | Overview |
| --- | --- |
| [FlowFuncLogger Constructor and Destructor](flowfunclogger-ctor-dtor.md) | FlowFuncLogger constructor and destructor. |
| [GetLogger](GetLogger.md) | Obtains the log implementation class. |
| [GetLogExtHeader](GetLogExtHeader.md) | Obtains the log extension header information. |
| [IsLogEnable](IsLogEnable.md) | Queries whether logs of the corresponding level and type are enabled. |
| [Error](Error.md) | Records ERROR-level logs. |
| [Warn](Warn.md) | Records a Warn-level log. |
| [Info](Info.md) | Records an Info-level log. |
| [Debug](Debug.md) | Records Debug-level logs. |
| [Error-Level Log Macro of Run Logs](run-log-error-level-log-macro.md) | Run log macro for the Error level. |
| [Info-Level Log Macro of Run Logs](run-log-info-level-log-macro.md) | Run log macro at the Info level. |
| [Error-Level Log Macro of Debug Logs](debug-log-error-level-log-macro.md) | Debug log macro for the Error level. |
| [Warn-Level Log Macro of Debug Logs](debug-log-warn-level-log-macro.md) | Debug log macro for Warn-level logs. |
| [Info-Level Log Macro of Debug Logs](debug-log-info-level-log-macro.md) | Debug log macro at the Info level. |
| [Debug-Level Log Macro of Debug Logs](debug-log-debug-level-log-macro.md) | Debug-level log macro. |

## Error Codes

**Table 18** Error codes

| Error code module | Overview |
| --- | --- |
| [flowfunc](udf-error-code.md#flowfunc) | Provides flowfunc error codes for users, mainly used for determining and handling exception logic. |
| [AICPU](udf-error-code.md#aicpu) | Error codes that AICPU may report to users during model execution. |
