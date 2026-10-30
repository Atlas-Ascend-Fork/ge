# DataFlow Header Files and Library Files

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-10-21T11:50:42.843Z pushedAt=2026-10-28T12:30:42.493Z nl=2 -->

You can view the header files of the corresponding APIs in the `${INSTALL_DIR}/include/flow_graph` and `${INSTALL_DIR}/include/ge` directories.

Replace `${INSTALL_DIR}` with the directory where the CANN software is installed. For example, if the Ascend-cann-toolkit package is installed by the root user, the installation directory is /usr/local/Ascend/cann.

| Interface Classification | Header File Path | Overview | Corresponding Library File |
| --- | --- | --- | --- |
| FlowOperator class | flow_graph.h | The FlowOperator class is the base class of DataFlow Graph nodes and inherits from the GE Operator. It cannot be constructed and used independently outside. | NA |
| FlowData class | flow_graph.h | Inherits From the FlowOperator class. It is a data node in a DataFlow Graph, and each FlowData corresponds to one input. | libflow_graph.so |
| FlowNode class | flow_graph.h | Inherits From the FlowOperator class. It is a compute node in a DataFlow Graph. | libflow_graph.so |
| FlowGraph class | flow_graph.h | A DataFlow graph, which consists of input nodes (FlowData) and compute nodes (FlowNode). | libflow_graph.so |
| ProcessPoint class | process_point.h | ProcessPoint is a virtual base class and cannot be instantiated. | NA |
| FunctionPp class | process_point.h | Inherits From the [ProcessPoint class](processpoint-class.md). It represents the compute processing point of a Function. | libflow_graph.so |
| GraphPp class | process_point.h | Inherits From the [ProcessPoint class](processpoint-class.md). It represents the compute processing point of a Graph. | libflow_graph.so |
| DataFlowInputAttr struct | flow_attr.h | Defines the timeBatch and countBatch functions to implement the UDF batching capability. | libflow_graph.so |
| Graph execution APIs | ge_api.h | Used to input data into a DataFlow graph and obtain the execution result of a DataFlow model. | libge_runner.so, libdavinci_executor.so, and libgraph_base.so |
| Data Type | ge_data_flow_api.h | Allows users to set and obtain the member variables in DataFlowInfo.<br> Note: If you compile the DataFlowInfo data type separately, you are advised to add -Wl,--no-as-needed to the compilation options to ensure that the symbols of the dependent .so files are fully loaded during compilation. | libdavinci_executor.so and libge_runner.so |

> [!NOTE]
> The char\_t type that appears in the header files is an alias of the char type.
