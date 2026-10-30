# DataFlow API List

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-21T11:45:42.625Z pushedAt=2026-10-28T12:30:42.473Z nl=2 -->

## DataFlow Graph Construction APIs

**Table 1** DataFlow graph construction APIs

| Interface Name | Overview |
| --- | --- |
| [FlowOperator Class](flowoperator-class.md) | The FlowOperator class is the base class of nodes in a DataFlow Graph, inherited from the Operator of GE. |
| [FlowData Constructor and Destructor](flowdata-ctor-dtor.md) | The constructor and destructor of FlowData. The constructor returns a FlowData node. |
| [FlowNode Constructor and Destructor](flownode-ctor-dtor.md) | The constructor and destructor of FlowNode. The constructor returns a FlowNode node. |
| [SetInput](SetInput.md) | Sets the input of a FlowNode, indicating that the src_index-th output of src_op is used as the dst_index-th input of the FlowNode, and returns the FlowNode node with the input set. |
| [AddPp](AddPp.md) | Adds a mapped ProcessPoint to a FlowNode. Currently, a FlowNode can add only one ProcessPoint. After the ProcessPoint is added, the inputs and outputs of the FlowNode are mapped to those of the ProcessPoint in order by default. |
| [MapInput](MapInput.md) | Maps the input of a FlowNode, indicating that the node_input_index-th input of the FlowNode is passed to the pp_input_index-th input of the ProcessPoint, and sets all properties in attrs for this input of the ProcessPoint. Returns the FlowNode node after mapping. This function is optional. If it is not called, the inputs of the FlowNode and the ProcessPoint are mapped in order by default. |
| [MapOutput](MapOutput.md) | Maps the output of a FlowNode, indicating that the pp_output_index-th output of the ProcessPoint is passed to the node_output_index-th output of the FlowNode, and returns the FlowNode node after mapping. |
| [SetBalanceScatter](SetBalanceScatter.md) | Sets the balance scatter property of a node. A UDF with the balance scatter property can use balance options to set load-balanced output. |
| [SetBalanceGather](SetBalanceGather.md) | Sets the balance gather property of a node. A UDF with the balance gather property can use balance options to set load-balanced affinity output. |
| [FlowGraph Constructor and Destructor](flowgraph-ctor-dtor.md) | The constructor and destructor of FlowGraph. The constructor returns an empty FlowGraph. |
| [SetInputs](SetInputs.md) | Sets the input nodes of a FlowGraph, automatically constructs a FlowGraph based on the output connection relationships of the nodes, and returns the graph. |
| [SetOutputs](SetOutputs.md) | Sets the output nodes of a FlowGraph and returns the graph. |
| [SetOutputs (index)](setoutputs-index.md) | Sets the association between a FlowNode in a FlowGraph and the output index of the FlowNode, and returns the graph. It is commonly used to set partial outputs of a FlowNode. For example, if FlowNode1 has two outputs but only one output of FlowNode1 is required as the input of FlowNode2, an output index of FlowNode1 can be set in this case. |
| [SetContainsNMappingNode](SetContainsNMappingNode.md) | Sets whether a FlowGraph contains an n_mapping node. |
| [SetInputsAlignAttrs](SetInputsAlignAttrs.md) | Sets the input alignment properties in a FlowGraph. |
| [const ge::Graph &ToGeGraph() const](const-ge-graph-togegraph-const.md) | Converts a FlowGraph to a Graph of GE. |
| [SetGraphPpBuilderAsync](SetGraphPpBuilderAsync.md) | Sets whether the Builder of GraphPp in a FlowGraph is executed asynchronously. |
| [SetExceptionCatch](SetExceptionCatch.md) | Sets whether the user exception catching function is enabled. |
| [ProcessPoint Destructor](processpoint-dtor.md) | The destructor of ProcessPoint. |
| [GetProcessPointType](GetProcessPointType.md) | Obtains the type of a ProcessPoint. |
| [GetProcessPointName](GetProcessPointName.md) | Obtains the name of a ProcessPoint. |
| [GetCompileConfig](GetCompileConfig.md) | Obtains the compilation configuration file of a ProcessPoint. |
| [Serialize (ProcessPoint Class)](serialize-processpoint-class.md) | The serialization method of ProcessPoint. The function of this method is implemented by subclasses of ProcessPoint. |
| [FunctionPp Constructor and Destructor](functionpp-ctor-dtor.md) | The constructor and destructor of FunctionPp. The constructor returns a FunctionPp object. |
| [SetCompileConfig (FunctionPp Class)](setcompileconfig-functionpp-class.md) | Sets the name and path of the JSON configuration file of FunctionPp. This configuration file is used to map FunctionPp to a UDF. |
| [AddInvokedClosure (Adding GraphPp to Be called)](addinvokedclosure-add-invoked-graph-pp.md) | Adds a GraphPp invoked by FunctionPp and returns the FunctionPp after the addition. |
| [AddInvokedClosure (Add the ProcessPoint Child Class to Be Called)](addinvokedclosure-add-invoked-processpoint-subclass.md) | Adds a GraphPp invoked by FunctionPp and returns the FunctionPp after the addition. |
| [AddInvokedClosure (Add FlowGraphPp to Be Called)](addinvokedclosure-add-invoked-flowgraph-pp.md) | Adds a FlowGraphPp invoked by FunctionPp and returns the FunctionPp after the addition. |
| [SetInitParam](SetInitParam.md) | Sets the initialization parameters of FunctionPp and returns the FunctionPp after the setting. |
| [Serialize (FunctionPp Class)](serialize-functionpp-class.md) | The serialization method of FunctionPp. |
| [GetInvokedClosures](GetInvokedClosures.md) | Obtains the GraphPp invoked by FunctionPp. |
| [GraphPp Constructor and Destructor](graphpp-ctor-dtor.md) | The constructor and destructor of GraphPp. The constructor returns a GraphPp object. |
| [SetCompileConfig (GraphPp Class)](setcompileconfig-graphpp-class.md) | Sets the path and file name of the JSON configuration file of GraphPp. The configuration file is used for the description and compilation of AscendGraph. |
| [Serialize (GraphPp Class)](serialize-graphpp-class.md) | The serialization method of GraphPp. |
| [GetGraphBuilder (GraphPp Class)](getgraphbuilder-graphpp-class.md) | Obtains the Graph creation function in GraphPp. |
| [FlowGraphPp Constructor and Destructor](flowgraphpp-ctor-dtor.md) | The constructor and destructor of FlowGraphPp. The constructor returns a FlowGraphPp object. |
| [Serialize (FlowGraphPp Class)](serialize-flowgraphpp-class.md) | The serialization method of FlowGraphPp. |
| [GetGraphBuilder (FlowGraphPp Class)](getgraphbuilder-flowgraphpp-class.md) | Obtains the Graph creation function in FlowGraphPp. |
| [TimeBatch](TimeBatch.md) | The TimeBatch function is based on UDF.<br>A normal model processes one piece of data at a time. When a batch of data needs to be processed at a time, the batch of data must be formed into a Batch. The most basic Batch method is to directly concatenate the N pieces of data and add N to the front of the shape. In some scenarios, data of a certain segment or several time segments needs to be formed into a batch and concatenated along a specific dimension. In this case, the TimeBatch function can be used to form the Batch. |
| [CountBatch](CountBatch.md) | The CountBatch function forms multiple pieces of data into a batch based on batchSize, using UDF as the computation processing point. |

## DataFlow Runtime APIs

**Table 2** DataFlow runtime APIs

| Interface Name | Overview |
| --- | --- |
| [FeedDataFlowGraph (Feeding All Inputs)](feeddataflowgraph-feed-all-input.md) | Feeds all data into the Graph. |
| [FeedDataFlowGraph (Feeding Input by Index)](feeddataflowgraph-feed-input-by-index.md) | Feeds data into the Graph by index. |
| [FeedDataFlowGraph (Feeding All FlowMsg)](feeddataflowgraph-feed-all-flowmsg.md) | Feeds data into the Graph. |
| [FeedDataFlowGraph (Feeding FlowMsg by Index)](feeddataflowgraph-feed-flowmsg-by-index.md) | Feeds data into the Graph by index. |
| [FeedRawData](FeedRawData.md) | Feeds raw data into the Graph. |
| [FetchDataFlowGraph (Obtaining All Output Data)](fetchdataflowgraph-get-all-output-data.md) | Obtains the graph output data. |
| [FetchDataFlowGraph (Obtaining Output Data by Index)](fetchdataflowgraph-get-output-data-by-index.md) | Obtains the graph output data by index. |
| [FetchDataFlowGraph (Obtaining All Output FlowMsg)](fetchdataflowgraph-get-all-output-flowmsg.md) | Obtains the graph output data. |
| [FetchDataFlowGraph (Obtaining Output FlowMsg by Index)](fetchdataflowgraph-get-output-flowmsg-by-index.md) | Obtains the graph output data by index. |
| [DataFlowInfo Data Type Constructor and Destructor](dataflowinfo-datatype-ctor-dtor.md) | DataFlowInfo constructor and destructor. |
| [SetUserData (DataFlowInfo Data Type)](setuserdata-dataflowinfo-datatype.md) | Sets user information. |
| [GetUserData (DataFlowInfo Data Type)](getuserdata-dataflowinfo-datatype.md) | Obtains user information. |
| [SetStartTime (DataFlowInfo Data Type)](setstarttime-dataflowinfo-datatype.md) | Sets the start timestamp of the data. |
| [GetStartTime (DataFlowInfo Data Type)](getstarttime-dataflowinfo-datatype.md) | Obtains the start timestamp of the data. |
| [SetEndTime (DataFlowInfo Data Type)](setendtime-dataflowinfo-datatype.md) | Sets the end timestamp of the data. |
| [GetEndTime (DataFlowInfo Data Type)](getendtime-dataflowinfo-datatype.md) | Obtains the end timestamp of the data. |
| [SetFlowFlags (DataFlowInfo Data Type)](setflowflags-dataflowinfo-datatype.md) | Sets the flags in the data. |
| [GetFlowFlags (DataFlowInfo Data Type)](getflowflags-dataflowinfo-datatype.md) | Obtains the flags in the data. |
| [SetTransactionId (DataFlowInfo Data Type)](settransactionid-dataflowinfo-datatype.md) | Sets the transaction ID used for DataFlow data transmission. |
| [GetTransactionId (DataFlowInfo Data Type)](gettransactionid-dataflowinfo-datatype.md) | Obtains the transaction ID used for DataFlow data transmission. |
| [FlowMsg Data Type Constructor and Destructor](flowmsg-datatype-ctor-dtor.md) | FlowMsg constructor and destructor. |
| [GetMsgType (FlowMsg Data Type)](getmsgtype-flowmsg-datatype.md) | Obtains the message type of FlowMsg. |
| [SetMsgType (FlowMsg Data Type)](setmsgtype-flowmsg-datatype.md) | Sets the message type of FlowMsg. |
| [GetTensor (FlowMsg Data Type)](gettensor-flowmsg-datatype.md) | Obtains the Tensor pointer in FlowMsg. |
| [GetRetCode (FlowMsg Data Type)](getretcode-flowmsg-datatype.md) | Obtains the error code in the input FlowMsg. |
| [SetRetCode (FlowMsg Data Type)](setretcode-flowmsg-datatype.md) | Sets the error code in FlowMsg. |
| [SetStartTime (FlowMsg Data Type)](setstarttime-flowmsg-datatype.md) | Sets the start timestamp in the FlowMsg message header. |
| [GetStartTime (FlowMsg Data Type)](getstarttime-flowmsg-datatype.md) | Obtains the start timestamp in the FlowMsg message. |
| [SetEndTime (FlowMsg Data Type)](setendtime-flowmsg-datatype.md) | Sets the end timestamp in the FlowMsg message header. |
| [GetEndTime (FlowMsg Data Type)](getendtime-flowmsg-datatype.md) | Obtains the end timestamp in the FlowMsg message. |
| [SetFlowFlags (FlowMsg Data Type)](setflowflags-flowmsg-datatype.md) | Sets the flags in the FlowMsg message header. |
| [GetFlowFlags (FlowMsg Data Type)](getflowflags-flowmsg-datatype.md) | Obtains the flags in the FlowMsg message header. |
| [GetTransactionId (FlowMsg Data Type)](gettransactionid-flowmsg-datatype.md) | Obtains the transaction ID in the FlowMsg message. |
| [SetTransactionId (FlowMsg Data Type)](settransactionid-flowmsg-datatype.md) | Sets the transaction ID in the FlowMsg message. |
| [SetUserData (FlowMsg Data Type)](setuserdata-flowmsg-datatype.md) | Sets user information. |
| [GetUserData (FlowMsg Data Type)](getuserdata-flowmsg-datatype.md) | Obtains user information. |
| [GetRawData (FlowMsg Data Type)](getrawdata-flowmsg-datatype.md) | Obtains the data pointer and data size corresponding to the data of the RawData type. |
| [AllocTensor (FlowBufferFactory Data Type)](alloctensor-flowbufferfactory-datatype.md) | Allocates a Tensor based on the shape, data type, and alignment size. |
| [AllocTensorMsg (FlowBufferFactory Data Type)](alloctensormsg-flowbufferfactory-datatype.md) | Allocates a FlowMsg based on the shape, data type, and alignment size. |
| [AllocRawDataMsg (FlowBufferFactory Data Type)](allocrawdatamsg-flowbufferfactory-datatype.md) | Allocates a contiguous memory block based on the input size to carry data of the raw data type. |
| [AllocEmptyDataMsg (FlowBufferFactory Data Type)](allocemptydatamsg-flowbufferfactory-datatype.md) | Allocates a message of the MsgType type for empty data. |
| [ToFlowMsg (Tensor)](to-flowmsg-tensor.md) | Converts the input Tensor into a FlowMsg that carries the Tensor. |
| [ToFlowMsg (Raw Data)](to-flowmsg-raw-data.md) | Converts the input raw data into a FlowMsg that carries the raw data. |
