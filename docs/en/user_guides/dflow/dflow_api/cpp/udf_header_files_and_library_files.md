# UDF Header Files and Library Files

<!-- md-trans-meta sourceCommit=8e99316f00319d82c87d56ea60105e7f826da7c5 translatedAt=2026-10-28T12:57:11.764Z pushedAt=2026-10-29T02:42:13.338Z nl=2 -->

This document describes the external APIs provided by the UDF (User Defined Function) module. You can call these APIs to develop custom processing functions, and then execute the processing functions on the CPU through DataFlow graph construction.

You can view the header files of the corresponding APIs in `${INSTALL_DIR}/include/flow_func`.

Replace `${INSTALL_DIR}` with the directory where the CANN software is installed. For example, if the Ascend-cann-toolkit package is installed by the root user, the installation directory is /usr/local/Ascend/cann.

**Table 1** Interface classification and corresponding header files

| Interface Classification | Header File Path | Overview | Corresponding Library File |
| --- | --- | --- |--- |
| AttrValue class | attr_value.h | Used to obtain the attribute values set by users. | libflow_func.so |
| AscendString class | ascend_string.h | Encapsulation of the String type. | libflow_func.so |
| MetaContext class | meta_context.h | Used for UDF context-related processing, such as applying for tensors and obtaining the set attributes. | libflow_func.so |
| FlowMsg class | flow_msg.h | Used for input and output related operations of flow func. | libflow_func.so |
| Tensor class | flow_msg.h | Used to perform Tensor-related operations. | - |
| MetaFlowFunc class | meta_flow_func.h | This class is defined in meta_flow_func.h. You can inherit this class to write a custom single-func processing function. | libflow_func.so |
| MetaMultiFunc class | meta_multi_func.h | This class is defined in meta_multi_func.h. You can inherit this class to write a custom multi-func processing function. | libflow_func.so |
| FlowFuncRegistrar class | meta_multi_func.h | This class is defined in meta_multi_func.h. It is an auxiliary template class for registering MetaMultiFunc. | libflow_func.so |
| MetaParams class | meta_params.h | This class is defined in meta_params.h. It is used in the multi-func processing function of FlowFunc to obtain shared variable information. | libflow_func.so |
| MetaRunContext class | meta_run_context.h | Used for context-related processing of the multi-func processing function of FlowFunc, such as applying for tensors, setting outputs, and running FlowModel. | libflow_func.so |
| OutOptions class | out_options.h | Provides a class for output options to carry related options when services publish data. | libflow_func.so |
| BalanceConfig class | balance_config.h | When balanced distribution is required, you need to set the output data identifier and weight matrix related configuration information. Based on the configuration, the scheduling module can complete balanced distribution among multiple instances. | libflow_func.so |
| FlowBufferFactory | flow_msg.h | - | libflow_func.so |
| FlowMsgQueue Class | flow_msg_queue.h | In streaming input scenarios (that is, when the input parameter of the flow func function is a queue), it is used as the input queue of flow func. The data objects in the queue are of the [FlowMsg class](flowmsg-class.md). | libflow_func.so |
| Registration macros | meta_flow_func.h<br>meta_multi_func.h | - | libflow_func.so |
| UDF log APIs | flow_func_log.h | flow_func_log.h provides log APIs to facilitate log recording during flow func development. | libflow_func.so |
| Error codes | flow_func_defines.h | - | libflow_func.so |
