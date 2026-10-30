# aclError

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:40:05.847Z pushedAt=2026-10-29T02:38:07.428Z nl=2 -->

```c
typedef int aclError;
```

Return code definition rules:

- Rule 1: For environment exceptions or code logic errors caused by developers, the problem can be resolved by optimizing the environment or code logic. The return code is defined as 1XXXXX at this time.
- Rule 2: For insufficient resources (Stream, memory, etc.) or mismatches between the APIs or parameters used by developers during programming and the current hardware, the problem can be resolved by using resources properly during programming. The return code is defined as 2XXXXX at this time.
- Rule 3: For service function exceptions, such as queue full or queue empty, the return code is defined as 3XXXXX at this time.
- Rule 4: For internal software and hardware exceptions, including internal software errors and Device execution failures, users cannot resolve the problem and need to report it to technical support. The return code is defined as 5XXXXX at this time. You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support.
- Rule 5: Unrecognizable errors are all mapped to 500000. You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support.

**Table 1**  ACL return code list

| Return Code | Meaning | Possible causes and solutions |
| --- | --- | --- |
| static const int ACL_SUCCESS = 0; | Executed successfully. | - |
| static const int ACL_ERROR_NONE = 0;<br>Note: This return code will be deprecated in later versions. Use the ACL_SUCCESS return code instead. | Executed successfully. | - |
| static const int ACL_ERROR_INVALID_PARAM = 100000; | Parameter validation failed. | Check whether the input parameter values of the API are correct. |
| static const int ACL_ERROR_UNINITIALIZE = 100001; | Not initialized. | - Check whether the aclInit API has been called for initialization. Ensure that the aclInit API has been called and is called before other ACL APIs.<br>  - Check whether the initialization API of the corresponding function has been called, for example, the aclmdlInitDump API for initializing Dump and the aclprofInit API for initializing Profiling. |
| static const int ACL_ERROR_REPEAT_INITIALIZE = 100002; | Repeated initialization or repeated loading. | Check whether the corresponding API is called for repeated initialization or repeated loading. |
| static const int ACL_ERROR_INVALID_FILE = 100003; | Invalid file. | Check whether the file exists and whether the file can be accessed. |
| static const int ACL_ERROR_WRITE_FILE = 100004; | Failed to write the file. | Check whether the file path exists and whether the file has write permission. |
| static const int ACL_ERROR_INVALID_FILE_SIZE = 100005; | Invalid file size. | Check whether the file size meets the API requirements. |
| static const int ACL_ERROR_PARSE_FILE = 100006; | Failed to parse the file. | Check whether the file content is valid. |
| static const int ACL_ERROR_FILE_MISSING_ATTR = 100007; | The file is missing parameters. | Check whether the file content is complete. |
| static const int ACL_ERROR_FILE_ATTR_INVALID = 100008; | Invalid file parameters. | Check whether the parameter values in the file are correct. |
| static const int ACL_ERROR_INVALID_DUMP_CONFIG = 100009; | Invalid Dump configuration. | Check whether the Dump configuration is correct. |
| static const int ACL_ERROR_INVALID_PROFILING_CONFIG = 100010; | Invalid Profiling configuration. | Check whether the Profiling configuration is correct. |
| static const int ACL_ERROR_INVALID_MODEL_ID = 100011; | Invalid model ID. | Check whether the model ID is correct and whether the model is loaded correctly. |
| static const int ACL_ERROR_DESERIALIZE_MODEL = 100012; | Failed to deserialize the model. | The model may mismatch the current version. Reconstruct the model. |
| static const int ACL_ERROR_PARSE_MODEL = 100013; | Failed to parse the model. | The model may mismatch the current version. Reconstruct the model. |
| static const int ACL_ERROR_READ_MODEL_FAILURE = 100014; | Failed to read the model. | Check whether the model file exists and whether the model file can be accessed. |
| static const int ACL_ERROR_MODEL_SIZE_INVALID = 100015; | Invalid model size. | The model file is invalid. Reconstruct the model. |
| static const int ACL_ERROR_MODEL_MISSING_ATTR = 100016; | The model is missing parameters. | The model may mismatch the current version. Reconstruct the model. |
| static const int ACL_ERROR_MODEL_INPUT_NOT_MATCH = 100017; | The model input does not match. | Check whether the model input is correct. |
| static const int ACL_ERROR_MODEL_OUTPUT_NOT_MATCH = 100018; | The model output does not match. | Check whether the model output is correct. |
| static const int ACL_ERROR_MODEL_NOT_DYNAMIC = 100019; | Not a dynamic model. | Check whether the current model supports dynamic scenarios. If not, reconstruct the model. |
| static const int ACL_ERROR_OP_TYPE_NOT_MATCH = 100020; | Single operator type mismatch. | Check whether the operator type is correct. |
| static const int ACL_ERROR_OP_INPUT_NOT_MATCH = 100021; | Single operator input mismatch. | Check whether the operator input is correct. |
| static const int ACL_ERROR_OP_OUTPUT_NOT_MATCH = 100022; | Single operator output mismatch. | Check whether the operator output is correct. |
| static const int ACL_ERROR_OP_ATTR_NOT_MATCH = 100023; | Single operator attribute mismatch. | Check whether the operator attributes are correct. |
| static const int ACL_ERROR_OP_NOT_FOUND = 100024; | Single operator not found. | Check whether the operator type is supported. |
| static const int ACL_ERROR_OP_LOAD_FAILED = 100025; | Failed to load the single operator. | The model may mismatch the current version. Reconstruct the single operator model. |
| static const int ACL_ERROR_UNSUPPORTED_DATA_TYPE = 100026; | Unsupported data type. | Check whether the data type exists or is currently supported. |
| static const int ACL_ERROR_FORMAT_NOT_MATCH = 100027; | Format mismatch. | Check whether the Format is correct. |
| static const int ACL_ERROR_BIN_SELECTOR_NOT_REGISTERED = 100028; | When compiling the operator API using the binary selection method, no selector is registered for the operator. | Check whether the aclopRegisterSelectKernelFunc API is called to register the operator selector. |
| static const int ACL_ERROR_KERNEL_NOT_FOUND = 100029; | When compiling the operator, the operator Kernel is not registered. | Check whether the aclopCreateKernel API is called to register the operator Kernel. |
| static const int ACL_ERROR_BIN_SELECTOR_ALREADY_REGISTERED = 100030; | When compiling the operator API using the binary selection method, the operator is registered repeatedly. | Check whether the aclopRegisterSelectKernelFunc API is called repeatedly to register the operator selector. |
| static const int ACL_ERROR_KERNEL_ALREADY_REGISTERED = 100031; | When compiling the operator, the operator Kernel is registered repeatedly. | Check whether the aclopCreateKernel API is called repeatedly to register the operator Kernel. |
| static const int ACL_ERROR_INVALID_QUEUE_ID = 100032; | Invalid queue ID. | Check whether the queue ID is correct. |
| static const int ACL_ERROR_REPEAT_SUBSCRIBE = 100033; | Repeated subscription. | Check whether the aclrtSubscribeReport API is called repeatedly for the same Stream. |
| static const int ACL_ERROR_STREAM_NOT_SUBSCRIBE = 100034;<br>Note: This return code will be deprecated in later versions. Use the ACL_ERROR_RT_STREAM_NO_CB_REG return code instead. | The Stream is not subscribed. | Check whether the aclrtSubscribeReport API has been called. |
| static const int ACL_ERROR_THREAD_NOT_SUBSCRIBE = 100035;<br>Note: This return code will be deprecated in later versions. Use the ACL_ERROR_RT_THREAD_SUBSCRIBE return code instead. | The thread is not subscribed. | Check whether the aclrtSubscribeReport API has been called. |
| static const int ACL_ERROR_WAIT_CALLBACK_TIMEOUT = 100036;<br>Note: This return code will be deprecated in later versions. Use the ACL_ERROR_RT_REPORT_TIMEOUT return code instead. | Timeout when waiting for the callback. | Check whether the aclrtLaunchCallback API has been called to deliver the callback task;<br>Check whether the timeout in the aclrtProcessReport API is reasonable;<br>Check whether the callback task has been processed. If it has been processed but the aclrtProcessReport API is still called, optimize the code logic. |
| static const int ACL_ERROR_REPEAT_FINALIZE = 100037; | Repeated deinitialization. | Check whether the aclFinalize API or the aclFinalizeReference API is called repeatedly for deinitialization. |
| static const int ACL_ERROR_NOT_STATIC_AIPP = 100038;<br>Note: This return code will be deprecated in later versions. Use the ACL_ERROR_GE_AIPP_NOT_EXIST return code instead. | The static AIPP configuration information does not exist. | When calling the aclmdlGetFirstAippInfo API, pass a correct index value. |
| static const int ACL_ERROR_COMPILING_STUB_MODE = 100039; | The dynamic library path configured before running the application is the path of the compilation stub, not the correct dynamic library path. | Check the dynamic library path configuration and ensure that the dynamic library in run mode is used. |
| static const int ACL_ERROR_GROUP_NOT_SET = 100040;<br>Note: This return code will be deprecated in later versions. Use the ACL_ERROR_RT_GROUP_NOT_SET return code instead. | The Group is not set. | Check whether the aclrtSetGroup API has been called. |
| static const int  ACL_ERROR_GROUP_NOT_CREATE = 100041;<br>Note: This return code will be deprecated in later versions. Use the ACL_ERROR_RT_GROUP_NOT_CREATE return code instead. | The corresponding Group is not created. | Check whether the Group ID set when calling the API is within the supported range. The value range of the Group ID is [0, (number of Groups - 1)]. You can call the aclrtGetGroupCount API to obtain the number of Groups. |
| static const int ACL_ERROR_PROF_ALREADY_RUN = 100042; | A task for collecting Profiling data already exists. | - Check the code logic. The configuration of "collecting Profiling data by calling AscendCL APIs" cannot coexist with Profiling configurations of other methods. Only one method can be retained.<br>  - Check whether multiple Profiling configurations are delivered repeatedly for the same Device. |
| static const int ACL_ERROR_PROF_NOT_RUN = 100043; | Profiling is not initialized using the aclprofInit API first. | Check the API call sequence. |
| static const int ACL_ERROR_DUMP_ALREADY_RUN = 100044; | A task for obtaining Dump data already exists. | Check whether the aclInit API has been called to configure Dump information before calling the aclmdlInitDump API, aclmdlSetDump API, and aclmdlFinalizeDump API to configure Dump information. If so, adjust the code logic and retain only one method to configure Dump information. |
| static const int ACL_ERROR_DUMP_NOT_RUN = 100045; | Dump is not initialized using the aclmdlInitDump API first. | Check the call sequence of the APIs for obtaining Dump data. See the description of the aclmdlInitDump API. |
| static const int ACL_ERROR_PROF_REPEAT_SUBSCRIBE = 148046; | The same model is subscribed repeatedly. | Check the API call sequence. |
| static const int ACL_ERROR_PROF_API_CONFLICT = 148047; | API call conflict when collecting performance data. | The Profiling performance data collection APIs of the two methods cannot be called in a cross manner. The aclprofModelSub API, aclprofGet* APIs, and aclprofModelUnSubscribe API cannot be called between the aclprofInit API and the aclprofFinalize API. The aclprofInit API, aclprofStart API, aclprofStop API, and aclprofFinalize API cannot be called between the aclprofModelSubscribe API and the aclprofModelUnSubscribe API. |
| static const int ACL_ERROR_INVALID_MAX_OPQUEUE_NUM_CONFIG = 148048; | Invalid operator cache information aging configuration. | Check the operator cache information aging configuration. See the configuration description and examples of aclInit. |
| static const int ACL_ERROR_INVALID_OPP_PATH = 148049; | The ASCEND_OPP_PATH environment variable is not set, or its value is set incorrectly. | Check whether the ASCEND_OPP_PATH environment variable is set and whether its value is the installation path of the opp software package. |
| static const int ACL_ERROR_OP_UNSUPPORTED_DYNAMIC = 148050; | The operator does not support dynamic Shape. | - Check whether the Shape of the operator in the single operator model file is dynamic. If it is dynamic, change it to a fixed Shape.<br>  - Check whether the Shape of aclTensorDesc is dynamic when compiling the operator. If it is dynamic, recreate aclTensorDesc with a fixed Shape. |
| static const int ACL_ERROR_RELATIVE_RESOURCE_NOT_CLEARED = 148051; | Related resources are not released. | When destroying the channel description information, this error code is returned if the related channel is not destroyed. Check whether the channel associated with the channel description information is destroyed. |
| static const int ACL_ERROR_UNSUPPORTED_JPEG = 148052; | The input image encoding format is not supported by the JPEGD function (for example, arithmetic coding, progressive coding). | When implementing the JPEGD image decoding function, only Huffman coding is supported. The color space of the original image before compression is YUV, and the component ratio of pixels is 4:4:4, 4:2:2, 4:2:0, 4:0:0, or 4:4:0. Arithmetic coding, progressive JPEG format, and JPEG2000 format are not supported. |
| static const int ACL_ERROR_INVALID_BUNDLE_MODEL_ID = 148053; | Invalid model ID. | Check whether the model ID is correct and whether the model is loaded correctly. |
| static const int ACL_ERROR_BAD_ALLOC = 200000; | Failed to allocate memory. | Check the remaining memory on the hardware environment. |
| static const int ACL_ERROR_API_NOT_SUPPORT = 200001; | The API is not supported. | Check whether the called API is currently supported. |
| static const int ACL_ERROR_INVALID_DEVICE = 200002;<br>Note: This return code will be deprecated in later versions. Use the ACL_ERROR_RT_INVALID_DEVICEID return code instead. | Invalid Device. | Check whether the Device exists. |
| static const int ACL_ERROR_MEMORY_ADDRESS_UNALIGNED = 200003; | The memory address is not aligned. | Check whether the memory address meets the API requirements. |
| static const int ACL_ERROR_RESOURCE_NOT_MATCH = 200004; | Resource mismatch. | Check whether correct resources such as Stream and Context are passed when calling the API. |
| static const int ACL_ERROR_INVALID_RESOURCE_HANDLE = 200005; | Invalid resource handle. | Check whether the resources such as Stream and Context passed when calling the API have been destroyed or occupied. |
| static const int ACL_ERROR_FEATURE_UNSUPPORTED = 200006; | The feature is not supported. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| static  const int ACL_ERROR_PROF_MODULES_UNSUPPORTED = 200007; | An unsupported Profiling configuration is delivered. | See the description in aclprofCreateConfig to check whether the Profiling configuration is correct. |
| static const int ACL_ERROR_STORAGE_OVER_LIMIT = 300000; | The storage limit is exceeded. | Check the remaining storage on the hardware environment. |
| static const int ACL_ERROR_INTERNAL_ERROR = 500000; | Unknown internal error. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| static const int ACL_ERROR_FAILURE = 500001; | Internal error. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| static const int ACL_ERROR_GE_FAILURE = 500002; | Error of the GE (Graph Engine) module. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| static const int ACL_ERROR_RT_FAILURE = 500003; | Error of the RUNTIME module. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| static const int ACL_ERROR_DRV_FAILURE = 500004; | Error of the Driver module. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| static const int ACL_ERROR_PROFILING_FAILURE = 500005; | Error of the Profiling module. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |

**Table 2**  Return code list passed through from RUNTIME

| Return Code | Meaning | Possible Causes and Solutions |
| --- | --- | --- |
| #define  ACL_RT_SUCCESS  0 | Executed successfully. | - |
| #define  ACL_ERROR_RT_PARAM_INVALID  107000 | Parameter validation failed. | Check whether the input parameters of the API are correct. |
| #define  ACL_ERROR_RT_INVALID_DEVICEID  107001 | Invalid Device ID. | Check whether the Device ID is valid. |
| #define  ACL_ERROR_RT_CONTEXT_NULL  107002 | The context is null. | Check whether aclrtSetCurrentContext or aclrtSetDevice has been called. |
| #define  ACL_ERROR_RT_STREAM_CONTEXT  107003 | The stream is not in the current context. | Check whether the context where the stream resides is consistent with the current context. |
| #define  ACL_ERROR_RT_MODEL_CONTEXT  107004 | The model is not in the current context. | Check whether the loaded model is consistent with the current context. |
| #define  ACL_ERROR_RT_STREAM_MODEL  107005 | The stream is not in the current model. | Check whether the stream has been bound to the model. |
| #define  ACL_ERROR_RT_EVENT_TIMESTAMP_INVALID  107006 | The event timestamp is invalid. | Check whether the event has been created. |
| #define  ACL_ERROR_RT_EVENT_TIMESTAMP_REVERSAL  107007 | The event timestamp is reversed. | Check whether the event has been created. |
| #define  ACL_ERROR_RT_ADDR_UNALIGNED  107008 | The memory address is not aligned. | Check whether the requested memory address is aligned. |
| #define  ACL_ERROR_RT_FILE_OPEN  107009 | Failed to open the file. | Check whether the file exists. |
| #define  ACL_ERROR_RT_FILE_WRITE  107010 | Failed to write the file. | Check whether the file exists or whether you have write permission. |
| #define  ACL_ERROR_RT_STREAM_SUBSCRIBE  107011 | The stream is not subscribed or is subscribed repeatedly. | Check whether the current stream is subscribed or subscribed repeatedly. |
| #define  ACL_ERROR_RT_THREAD_SUBSCRIBE  107012 | The thread is not subscribed or is subscribed repeatedly. | Check whether the current thread is subscribed or subscribed repeatedly. |
| #define  ACL_ERROR_RT_GROUP_NOT_SET  107013 | No group is set. | Check whether the aclrtSetGroup API has been called. |
| #define  ACL_ERROR_RT_GROUP_NOT_CREATE  107014 | The corresponding group is not created. | Check whether the Group ID set when calling the API is within the supported range. The value range of Group ID is [0, (number of groups - 1)]. You can call the aclrtGetGroupCount API to obtain the number of groups. |
| #define  ACL_ERROR_RT_STREAM_NO_CB_REG  107015 | The stream corresponding to the callback is not registered to the thread. | Check whether the stream has been registered to the thread and whether the aclrtSubscribeReport API has been called. |
| #define  ACL_ERROR_RT_INVALID_MEMORY_TYPE  107016 | Invalid memory type. | Check whether the memory type is valid. |
| #define  ACL_ERROR_RT_INVALID_HANDLE  107017 | Invalid resource handle. | Check whether the corresponding input and used parameters are correct. |
| #define  ACL_ERROR_RT_INVALID_MALLOC_TYPE  107018 | The memory type used for allocation is incorrect. | Check whether the corresponding input and used memory type is correct. |
| #define  ACL_ERROR_RT_WAIT_TIMEOUT  107019 | Wait timed out. | Try to re-execute the API that delivers the task. |
| #define  ACL_ERROR_RT_TASK_TIMEOUT  107020 | Task execution timed out. | Check whether the service orchestration is reasonable or set a reasonable timeout period. |
| #define  ACL_ERROR_RT_SYSPARAMOPT_NOT_SET  107021 | Failed to obtain the system parameter value in the current context. | The system parameter value in the current context is not set. See aclrtCtxSetSysParamOpt. |
| #define  ACL_ERROR_RT_DEVICE_TASK_ABORT  107022 | The task abort operation on the Device conflicts with other operations. | This return code is caused by a conflict between calling the aclrtDeviceTaskAbort API to stop tasks on the Device and other API operations. Check the code logic and perform other operations only after the aclrtDeviceTaskAbort API has finished executing. |
| #define  ACL_ERROR_RT_STREAM_ABORT  107023 | Tasks on the Stream are being cleared. | Tasks on the specified Stream are being cleared. Synchronous waiting for task execution on the Stream is not supported. |
| #define  ACL_ERROR_RT_CAPTURE_DEPENDENCY  107024 | When building a model running instance based on capture, the event wait task has no corresponding event record task. | Call the aclrtRecordEvent API to add an event record task. |
| #define  ACL_ERROR_RT_STREAM_UNJOINED  107025 | In a cross-stream dependency scenario, the captured model contains other Streams that are not merged into the original Stream, causing errors on the other Streams. | - The last task in the Stream that reports the error is not an event record task. Insert an event record task that can be merged into the original Stream at the end of the Stream that reports the error (call the aclrtRecordEvent API).<br>  - The last event record task has no corresponding event wait task. Insert an event wait task on the Stream that does not report the error (call the aclrtStreamWaitEvent API) to ensure that the event wait task can merge other Streams into the original Stream. |
| #define  ACL_ERROR_RT_MODEL_CAPTURED  107026 | The model is already in the captured state. | Call the aclmdlRICaptureEnd API to end the capture first, and then perform the corresponding operation. |
| #define  ACL_ERROR_RT_STREAM_CAPTURED  107027 | The Stream is already in the captured state. | Call the aclmdlRICaptureEnd API to end the capture first, and then perform the corresponding operation. |
| #define  ACL_ERROR_RT_EVENT_CAPTURED  107028 | The Event is already in the captured state. | Call the aclmdlRICaptureEnd API to end the capture first, and then perform the corresponding operation. |
| #define  ACL_ERROR_RT_STREAM_NOT_CAPTURED  107029 | The Stream is not in the captured state. | Check whether aclmdlRICaptureBegin has been called to start capturing tasks delivered on the Stream. |
| #define  ACL_ERROR_RT_CAPTURE_MODE_NOT_SUPPORT  107030 | The current capture mode does not support this operation. | Different capture modes support different operation scopes. See the description in the aclmdlRICaptureThreadExchangeMode API and switch to the correct capture mode. |
| #define  ACL_ERROR_RT_STREAM_CAPTURE_IMPLICIT  107031 | The default Stream is not allowed in the capture scenario. | Try to use another Stream instead of the default Stream. |
| #define  ACL_ERROR_STREAM_CAPTURE_CONFLICT  107032 | The model running instances where the Event and the Stream reside are inconsistent. | Check whether the multiple Streams that call the aclmdlRICaptureBegin operation simultaneously are associated through Events. |
| #define  ACL_ERROR_STREAM_TASK_GROUP_STATUS  107033 | The task group status is abnormal. | The possible causes are as follows. Check them one by one.<br><br>  - aclmdlRICaptureTaskGrpBegin is called to repeatedly start task group recording on the same Stream.<br>  - aclmdlRICaptureTaskUpdateBegin is called to update the same task group simultaneously.<br>  - aclmdlRICaptureTaskGrpBegin is not used in pair with aclmdlRICaptureTaskGrpEnd.<br>  - aclmdlRICaptureTaskUpdateBegin is not used in pair with aclmdlRICaptureTaskUpdateEnd.<br>  - The same Stream is used to record and update task groups simultaneously. |
| #define  ACL_ERROR_STREAM_TASK_GROUP_INTR  107034 | The task group recording process is interrupted. | This error occurs because tasks are submitted to a capture stream on which task group recording is not enabled during task group recording.<br>It is recommended to call the aclmdlRICaptureTaskUpdateBegin API to enable task group recording first, and then submit the tasks. |
| #define  ACL_ERROR_RT_STREAM_CAPTURE_UNMATCHED  107036 | An unmatched operation is found during capture, and aclmdlRICaptureBegin is not called to start capturing tasks on the Stream. | Check the code logic. Call the aclmdlRICaptureBegin API to start capturing tasks on the Stream first, and then call the aclmdlRICaptureEnd API to end the capture. |
| #define  ACL_ERROR_RT_MODEL_RUNNING  107037 | The current model is being executed and cannot be destroyed. | Check the code logic. Destroy the model only after the model execution is complete. |
| #define  ACL_ERROR_RT_STREAM_CAPTURE_WRONG_THREAD  107038 | aclmdlRICaptureEnd and aclmdlRICaptureBegin are not in the same thread. | In the aclmdlRICaptureBegin API, if mode is set to a value other than ACL_MODEL_RI_CAPTURE_MODE_RELAXED, the aclmdlRICaptureEnd API and the aclmdlRICaptureBegin API must be in the same thread. |
| #define  ACL_ERROR_RT_INSUFFICIENT_INPUT_ARRAY  107039 | Insufficient array space. The array size is smaller than the actually required array size. | Adjust the input array size. |
| #define  ACL_ERROR_RT_MODEL_UPDATE_FAILED  107040 | The model is not fully captured, the model is being executed, or parameter update fails, causing the model update to fail. | Ensure that the model has been successfully captured, execution is complete, or parameters are updated successfully. |
| #define ACL_ERROR_RT_CAPTURE_MODE_BLOCK_ASYNC 107041 | The current capture mode does not support the asynchronous-to-synchronous conversion operation. | Different capture modes support different operation scopes. See the description in the aclmdlRICaptureThreadExchangeMode API and switch to the correct capture mode. |
| #define  ACL_ERROR_RT_SYMBOL_NOT_FOUND  107042 | Symbol not found. | Check whether the global variable name is consistent with the global variable name in the operator binary. |
| #define  ACL_ERROR_RT_INVALID_SYMBOL  107043 | Invalid symbol. | Check whether the variable name is declared in the Device code. |
| #define  ACL_ERROR_RT_INVALID_DEVICE_FUNCTION  107044 | Invalid kernel function symbol. | Check whether the kernel function is declared in the Device code. |
| #define  ACL_ERROR_RT_FEATURE_NOT_SUPPORT  207000 | The feature is not supported. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_MEMORY_ALLOCATION  207001 | Memory allocation failed. | Check the remaining storage on the hardware environment. |
| #define  ACL_ERROR_RT_MEMORY_FREE  207002 | Memory release failed. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICORE_OVER_FLOW  207003 | AI Core operator computation overflow. | Check whether the corresponding AI Core operator computation overflows. You can find the overflowing operator based on the dump data and then locate the specific operator problem. |
| #define  ACL_ERROR_RT_NO_DEVICE  207004 | The Device is unavailable. | Check whether the Device is running properly. |
| #define  ACL_ERROR_RT_RESOURCE_ALLOC_FAIL  207005 | Resource allocation failed. | Check the usage of resources such as Streams and release unused resources in a timely manner. |
| #define  ACL_ERROR_RT_NO_PERMISSION  207006 | No operation permission. | Check whether the user permission for running the application is correct. |
| #define  ACL_ERROR_RT_NO_EVENT_RESOURCE  207007 | Insufficient Event resources. | See the description at the aclrtCreateEvent API to check whether the number of Events meets the requirements. |
| #define  ACL_ERROR_RT_NO_STREAM_RESOURCE  207008 | Insufficient Stream resources. | See the description at the aclrtCreateStream API to check whether the number of Streams meets the requirements. |
| #define  ACL_ERROR_RT_NO_NOTIFY_RESOURCE  207009 | Insufficient internal Notify resources in the system. | Too many concurrent tasks for media data processing or too many resources consumed during model inference. It is recommended to reduce concurrent tasks or unload some models. |
| #define  ACL_ERROR_RT_NO_MODEL_RESOURCE  207010 | Insufficient model resources. | It is recommended to unload some models. |
| #define  ACL_ERROR_RT_NO_CDQ_RESOURCE  207011 | Insufficient internal Runtime resources. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_OVER_LIMIT  207012 | The number of queues exceeds the upper limit. | Destroy the queues that are no longer needed before creating new queues. |
| #define  ACL_ERROR_RT_QUEUE_EMPTY  207013 | The queue is empty. | Data cannot be obtained from an empty queue. Add data to the queue first, and then obtain it. |
| #define  ACL_ERROR_RT_QUEUE_FULL  207014 | The queue is full. | Data cannot be added to a full queue. Obtain data from the queue first, and then add it. |
| #define  ACL_ERROR_RT_REPEATED_INIT  207015 | The queue is initialized repeatedly. | It is recommended to initialize the queue only once. Do not initialize it repeatedly. |
| #define  ACL_ERROR_RT_DEVICE_OOM  207018 | The Device-side memory is exhausted. | Check the memory usage on the Device and plan the memory usage properly based on the memory specifications on the Device. |
| #define  ACL_ERROR_RT_FEATURE_NOT_SUPPORT_UPDATE_OP  207019 | The current driver version does not support updating this operator. | Check the driver version.<br>You can click [Link](https://www.hiascend.com/hardware/firmware-drivers/commercial), download the driver installation package of Ascend HDK 25.0.RC1 or later on the "Firmware and Driver" page, and install or upgrade it by referring to the documentation of the corresponding version. |
| #define  ACL_ERROR_RT_INTERNAL_ERROR  507000 | Internal error in the runtime module. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_TS_ERROR  507001 | Internal error in the task scheduler module on the Device. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_STREAM_TASK_FULL  507002 | The number of tasks on the stream is full. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_STREAM_TASK_EMPTY  507003 | The number of tasks on the stream is empty. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_STREAM_NOT_COMPLETE  507004 | Not all tasks on the stream have been executed. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_END_OF_SEQUENCE  507005 | The task on the AI CPU has been executed. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_EVENT_NOT_COMPLETE  507006 | The event is not complete. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_CONTEXT_RELEASE_ERROR  507007 | Failed to release the context. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_SOC_VERSION  507008 | Failed to obtain the soc version. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_TASK_TYPE_NOT_SUPPORT  507009 | Unsupported task type. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_LOST_HEARTBEAT  507010 | The task scheduler lost the heartbeat. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_MODEL_EXECUTE  507011 | Model execution failed. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_REPORT_TIMEOUT  507012 | Failed to obtain the message from the task scheduler. | Check whether the timeout period of the API is set too short, and increase it appropriately. If the timeout error persists after increasing the timeout period, check the log.<br>You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_SYS_DMA  507013 | System DMA (Direct Memory Access) hardware execution error. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICORE_TIMEOUT  507014 | AI Core execution timed out. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support.<br>For details about the log, see *Log Reference*. |
| #define  ACL_ERROR_RT_AICORE_EXCEPTION  507015 | AI Core execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICORE_TRAP_EXCEPTION  507016 | AI Core trap execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICPU_TIMEOUT  507017 | AI CPU execution timed out. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICPU_EXCEPTION  507018 | AI CPU execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICPU_DATADUMP_RSP_ERR  507019 | The AI CPU did not return a response to the task scheduler after dumping data. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICPU_MODEL_RSP_ERR  507020 | The AI CPU did not return a response to the task scheduler after executing the model. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_PROFILING_ERROR  507021 | Profiling execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_IPC_ERROR  507022 | Inter-process communication exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_MODEL_ABORT_NORMAL  507023 | The model exited. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_KERNEL_UNREGISTERING  507024 | The operator is being unregistered. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_RINGBUFFER_NOT_INIT  507025 | The ringbuffer (ring buffer) function is not initialized. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_RINGBUFFER_NO_DATA  507026 | The ringbuffer (ring buffer) has no data. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_KERNEL_LOOKUP  507027 | The kernel inside RUNTIME is not registered. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_KERNEL_DUPLICATE  507028 | The kernel inside RUNTIME is registered repeatedly. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_DEBUG_REGISTER_FAIL  507029 | Failed to register the debug function. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_DEBUG_UNREGISTER_FAIL  507030 | Failed to unregister the debug function. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_LABEL_CONTEXT  507031 | The label is not in the current context. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_PROGRAM_USE_OUT  507032 | The number of registered programs exceeds the limit. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_DEV_SETUP_ERROR  507033 | Failed to start the Device. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_VECTOR_CORE_TIMEOUT  507034 | Vector core execution timed out. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_VECTOR_CORE_EXCEPTION  507035 | Vector core execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_VECTOR_CORE_TRAP_EXCEPTION  507036 | Vector core trap execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_CDQ_BATCH_ABNORMAL  507037 | Runtime internal resource application exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_DIE_MODE_CHANGE_ERROR  507038 | Failed to change the die mode. The die mode cannot be changed. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_DIE_SET_ERROR  507039 | The die cannot be specified in single-die mode. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_INVALID_DIEID  507040 | The specified die ID is incorrect. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_DIE_MODE_NOT_SET  507041 | The die mode is not set. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICORE_TRAP_READ_OVERFLOW  507042 | AI Core trap read out-of-bounds exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICORE_TRAP_WRITE_OVERFLOW  507043 | AI Core trap write out-of-bounds exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_VECTOR_CORE_TRAP_READ_OVERFLOW  507044 | Vector Core trap read out-of-bounds exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_VECTOR_CORE_TRAP_WRITE_OVERFLOW  507045 | Vector Core trap write out-of-bounds exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_STREAM_SYNC_TIMEOUT  507046 | Within the specified timeout wait event, not all tasks in the specified stream have completed execution. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_EVENT_SYNC_TIMEOUT  507047 | During the specified Event synchronization wait, the Event has not completed execution within the specified time. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_FFTS_PLUS_TIMEOUT  507048 | Internal task execution timed out. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_FFTS_PLUS_EXCEPTION  507049 | Internal task execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_FFTS_PLUS_TRAP_EXCEPTION  507050 | Internal task trap exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_SEND_MSG  507051 | Failed to send a message during data enqueueing. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_COPY_DATA  507052 | Failed to copy memory during data enqueueing. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_DEVICE_MEM_ERROR  507053 | An erroneous virtual address of a memory UCE (Uncorrectable error, which means the system hardware cannot directly handle and recover the memory error) occurred. | See the description in the aclrtGetMemUceInfo API to obtain and fix the erroneous virtual address of the memory UCE. |
| #define  ACL_ERROR_RT_HBM_MULTI_BIT_ECC_ERROR  507054 | HBM multi-bit ECC fault. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_SUSPECT_DEVICE_MEM_ERROR  507055 | In multi-process and multi-Device scenarios, a memory UCE error may occur. | Because the memory of the peer Device accessed by the current Device is faulty, you need to check the error information of the peer Device process. |
| #define  ACL_ERROR_RT_LINK_ERROR  507056 | In multi-Device scenarios, the communication link between two Devices is disconnected. | Retry is recommended. If the error persists, check the communication link between the two Devices. |
| #define  ACL_ERROR_RT_SUSPECT_REMOTE_ERROR  507057 | In multi-process and multi-Device scenarios, the memory of the peer Device may be faulty, or the memory access of the current Device is out of bounds. | You need to check the error information of the peer Device process or the memory access of the current Device. |
| #define  ACL_ERROR_RT_DRV_INTERNAL_ERROR  507899 | Internal error in the Driver module. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICPU_INTERNAL_ERROR  507900 | Internal error in the AI CPU module. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_SOCKET_CLOSE  507901 | The internal HDC (Host Device Communication) session link is disconnected. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_AICPU_INFO_LOAD_RSP_ERR  507902 | AI CPU scheduling failed. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_STREAM_CAPTURE_INVALIDATED  507903 | Model capture exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define ACL_ERROR_RT_COMM_OP_RETRY_FAIL  507904 | Failed to re-execute the communication operator. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_SNAPSHOT_LOCK_TIMEOUT  507905 | The aclrtSnapShotProcessLock API timed out. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_SNAPSHOT_LOCK_FAILED  507906 | Failed to lock the current process. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_SNAPSHOT_UNLOCK_FAILED  507907 | Failed to unlock the current process. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_SNAPSHOT_BACKUP_FAILED  507908 | Failed to back up the snapshot process. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_SNAPSHOT_RESTORE_FAILED  507909 | Failed to restore the snapshot process. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_HOST_MEMORY_ALREADY_REGISTERED  507910 | The host memory has already been registered. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_HOST_MEMORY_NOT_REGISTERED  507911 | The host memory to be unregistered has not been registered. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_SNAPSHOT_CALLBACK_FAILED  507912 | Failed to execute the callback function at a certain snapshot stage. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_SNAPSHOT_REGISTER_CALLBACK_FAILED<br>507913 | Failed to register the callback function. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_L3_PORT_ERROR  507914 | L3 port error. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_CCU_TIMEOUT  507915 | CCU (Collective Communication Unit) execution timed out. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| #define  ACL_ERROR_RT_CCU_EXCEPTION  507916 | CCU (Collective Communication Unit) execution exception. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |

**Table 3**  Return codes passed through from GE

| Return Code | Meaning | Possible causes and solutions |
| --- | --- | --- |
| uint32_t ACL_ERROR_GE_PARAM_INVALID = 145000; | Parameter validation failed. | Check whether the input parameter values of the API are correct. |
| uint32_t ACL_ERROR_GE_EXEC_NOT_INIT = 145001; | Not initialized. | - Check whether the aclInit API has been called for initialization. Ensure that the aclInit API has been called and is called before other ACL APIs.<br>  - Check whether the initialization API of the corresponding function has been called, for example, the aclmdlInitDump API for initializing Dump and the aclprofInit API for initializing Profiling. |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID = 145002; | Invalid model path. | Check whether the model path is correct. |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_ID_INVALID = 145003; | Invalid model ID. | Check whether the model ID is correct and whether the model is loaded correctly. |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID = 145006; | Invalid model size. | The model file is invalid. Reconstruct the model. |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID = 145007; | Invalid model memory address. | Check whether the model address is valid. |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID = 145008; | Invalid queue ID. | Check whether the queue ID is correct. |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_MODEL_REPEATED = 145009; | Repeated initialization or repeated loading. | Check whether the corresponding API is called for repeated initialization or repeated loading. |
| uint32_t ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID = 145011; | Invalid dynamic profiling input address. | Check the dynamic profiling input address. |
| uint32_t ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID = 145012; | Invalid dynamic profiling input length. | Check the dynamic profiling input length. |
| uint32_t ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID = 145013; | Invalid dynamic profiling batch size. | Check the dynamic profiling batch size. |
| uint32_t ACL_ERROR_GE_AIPP_BATCH_EMPTY = 145014; | Invalid AIPP batch size. | Check whether the AIPP batch size is correct. |
| uint32_t ACL_ERROR_GE_AIPP_NOT_EXIST = 145015; | AIPP configuration does not exist. | Check whether AIPP is configured. |
| uint32_t ACL_ERROR_GE_AIPP_MODE_INVALID = 145016; | Invalid AIPP mode. | Check whether the AIPP mode configured during model conversion is correct. |
| uint32_t ACL_ERROR_GE_OP_TASK_TYPE_INVALID = 145017; | Invalid task type. | Check whether the operator type is correct. |
| uint32_t ACL_ERROR_GE_OP_KERNEL_TYPE_INVALID = 145018; | Invalid operator type. | Check whether the operator type is correct. |
| uint32_t ACL_ERROR_GE_PLGMGR_PATH_INVALID = 145019; | Invalid .so file, including cases such as the .so file path being too deep or the .so file being deleted by mistake. | Check whether the LD_LIBRARY_PATH environment variable configured before running the application is correct. For details, see the operation guide in the compilation and running section. |
| uint32_t ACL_ERROR_GE_FORMAT_INVALID = 145020; | Invalid format. | Check whether the format of the tensor data is valid. |
| uint32_t ACL_ERROR_GE_SHAPE_INVALID = 145021; | Invalid shape. | Check whether the shape of the tensor data is valid. |
| uint32_t ACL_ERROR_GE_DATATYPE_INVALID = 145022; | Invalid data type. | Check whether the data type of the tensor data is valid. |
| uint32_t ACL_ERROR_GE_MEMORY_ALLOCATION = 245000; | Failed to allocate memory. | Check the remaining memory on the hardware environment. |
| uint32_t ACL_ERROR_GE_MEMORY_OPERATE_FAILED = 245001; | Memory initialization or memory copy operation failed. | Check whether the memory address is correct and whether the memory on the hardware environment is sufficient. |
| uint32_t ACL_ERROR_GE_DEVICE_MEMORY_ALLOCATION_FAILED = 245002; | Failed to allocate Device memory. | The Device memory is exhausted and cannot be allocated further. Release some Device memory and try again. |
| uint32_t  ACL_ERROR_GE_SUBHEALTHY = 345102; | Sub-healthy state. | The sub-healthy state is the state after the redeployment action triggered by a device or process exception is completed. In the sub-healthy state, related APIs can be called normally. |
| static const uint32_t ACL_ERROR_GE_USER_RAISE_EXCEPTION = 345103; | A user-defined function actively throws an exception. | You can identify which input data caused the execution error based on the UserData set in DataFlowInfo, and then troubleshoot the problem based on the error. |
| static const uint32_t ACL_ERROR_GE_DATA_NOT_ALIGNED = 345104; | Data is not aligned. | If a user-defined function has multiple outputs, check whether the user code has set fewer outputs. Missing outputs may cause data alignment exceptions. |
| uint32_t ACL_ERROR_GE_INTERNAL_ERROR = 545000; | Unknown internal error. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_LOAD_MODEL = 545001; | Failed to load the model internally in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_MODEL_PARTITION_FAILED = 545002; | Failed to load the model internally in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_WEIGHT_PARTITION_FAILED = 545003; | Failed to load the model weights internally in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_TASK_PARTITION_FAILED = 545004; | Failed to load the model task internally in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_KERNEL_PARTITION_FAILED = 545005; | Failed to load the model operator internally in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_EXEC_RELEASE_MODEL_DATA = 545006; | Failed to release the model space in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_COMMAND_HANDLE = 545007; | Command operation failed in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_GET_TENSOR_INFO = 545008; | Failed to obtain tensor data in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_UNLOAD_MODEL = 545009; | Failed to unload the model space in the system. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_MODEL_EXECUTE_TIMEOUT = 545601; | Model execution timed out. | You can obtain the log and click [Link](https://www.hiascend.com/support) to contact technical support. |
| uint32_t ACL_ERROR_GE_REDEPLOYING = 545602; | Redeploying. | Wait for the redeployment action to complete and then call the related APIs again. |
