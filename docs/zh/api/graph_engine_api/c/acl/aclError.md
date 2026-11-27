# aclError

```c
typedef int aclError;
```

Runtime返回码请参见《[Runtime运行时API](https://gitcode.com/cann/runtime/blob/master/docs/zh/api_ref/README.md)》。

返回码定义规则：

- 规则1：开发人员的环境异常或者代码逻辑错误，可以通过优化环境或代码逻辑的方式解决问题，此时返回码定义为1XXXXX。
- 规则2：资源不足（Stream、内存等）、开发人员编程时使用的接口或参数与当前硬件不匹配，可以通过在编程时合理使用资源的方式解决，此时返回码定义为2XXXXX。
- 规则3：业务功能异常，比如队列满、队列空等，此时返回码定义为3XXXXX。
- 规则4：软硬件内部异常，包括软件内部错误、Device执行失败等，用户无法解决问题，需要将问题反馈给技术支持，此时返回码定义为5XXXXX。您可以获取日志后提交ISSUE。
- 规则5：无法识别的错误，当前都映射为500000。您可以获取日志后提交ISSUE。

**表 1**  acl返回码列表

| 返回码 | 含义 | 可能原因及解决方法 |
| --- | --- | --- |
| static const int ACL_SUCCESS = 0; | 执行成功。 | - |
| static const int ACL_SUCCESS = 0;<br>须知：此返回码后续版本会废弃，请使用ACL_SUCCESS返回码。 | 执行成功。 | - |
| static const int ACL_ERROR_INVALID_PARAM = 100000; | 参数校验失败。 | 请检查接口的入参值是否正确。 |
| static const int ACL_ERROR_UNINITIALIZE = 100001; | 未初始化。 | - 请检查是否已调用aclInit接口进行初始化，请确保已调用aclInit接口，且在其它acl接口之前调用。<br>  - 请检查是否已调用对应功能的初始化接口，例如初始化Dump的aclmdlInitDump接口、初始化Profiling的aclprofInit接口。 |
| static const int ACL_ERROR_REPEAT_INITIALIZE = 100002; | 重复初始化或重复加载。 | 请检查是否调用对应的接口重复初始化或重复加载。 |
| static const int ACL_ERROR_INVALID_FILE = 100003; | 无效的文件。 | 请检查文件是否存在、文件是否能被访问等。 |
| static const int ACL_ERROR_WRITE_FILE = 100004; | 写文件失败。 | 请检查文件路径是否存在、文件是否有写权限等。 |
| static const int ACL_ERROR_INVALID_FILE_SIZE = 100005; | 无效的文件大小。 | 请检查文件大小是否符合接口要求。 |
| static const int ACL_ERROR_PARSE_FILE = 100006; | 解析文件失败。 | 请检查文件内容是否合法。 |
| static const int ACL_ERROR_FILE_MISSING_ATTR = 100007; | 文件缺失参数。 | 请检查文件内容是否完整。 |
| static const int ACL_ERROR_FILE_ATTR_INVALID = 100008; | 文件参数无效。 | 请检查文件中参数值是否正确。 |
| static const int ACL_ERROR_INVALID_DUMP_CONFIG = 100009; | 无效的Dump配置。 | 请检查Dump配置是否正确。 |
| static const int ACL_ERROR_INVALID_PROFILING_CONFIG = 100010; | 无效的Profiling配置。 | 请检查Profiling配置是否正确。 |
| static const int ACL_ERROR_INVALID_MODEL_ID = 100011; | 无效的模型ID。 | 请检查模型ID是否正确、模型是否正确加载。 |
| static const int ACL_ERROR_DESERIALIZE_MODEL = 100012; | 反序列化模型失败。 | 模型可能与当前版本不匹配，请重新构建模型。 |
| static const int ACL_ERROR_PARSE_MODEL = 100013; | 解析模型失败。 | 模型可能与当前版本不匹配，请重新构建模型。 |
| static const int ACL_ERROR_READ_MODEL_FAILURE = 100014; | 读取模型失败。 | 请检查模型文件是否存在、模型文件是否能被访问等。 |
| static const int ACL_ERROR_MODEL_SIZE_INVALID = 100015; | 无效的模型大小。 | 模型文件无效，请重新构建模型。 |
| static const int ACL_ERROR_MODEL_MISSING_ATTR = 100016; | 模型缺少参数。 | 模型可能与当前版本不匹配，请重新构建模型。 |
| static const int ACL_ERROR_MODEL_INPUT_NOT_MATCH = 100017; | 模型的输入不匹配。 | 请检查模型的输入是否正确。 |
| static const int ACL_ERROR_MODEL_OUTPUT_NOT_MATCH = 100018; | 模型的输出不匹配。 | 请检查模型的输出是否正确。 |
| static const int ACL_ERROR_MODEL_NOT_DYNAMIC = 100019; | 非动态模型。 | 请检查当前模型是否支持动态场景，如不支持，请重新构建模型。 |
| static const int ACL_ERROR_OP_TYPE_NOT_MATCH = 100020; | 单算子类型不匹配。 | 请检查算子类型是否正确。 |
| static const int ACL_ERROR_OP_INPUT_NOT_MATCH = 100021; | 单算子的输入不匹配。 | 请检查算子的输入是否正确。 |
| static const int ACL_ERROR_OP_OUTPUT_NOT_MATCH = 100022; | 单算子的输出不匹配。 | 请检查算子的输出是否正确。 |
| static const int ACL_ERROR_OP_ATTR_NOT_MATCH = 100023; | 单算子的属性不匹配。 | 请检查算子的属性是否正确。 |
| static const int ACL_ERROR_OP_NOT_FOUND = 100024; | 单算子未找到。 | 请检查算子类型是否支持。 |
| static const int ACL_ERROR_OP_LOAD_FAILED = 100025; | 单算子加载失败。 | 模型可能与当前版本不匹配，请重新构建单算子模型。 |
| static const int ACL_ERROR_UNSUPPORTED_DATA_TYPE = 100026; | 不支持的数据类型。 | 请检查数据类型是否存在或当前是否支持。 |
| static const int ACL_ERROR_FORMAT_NOT_MATCH = 100027; | Format不匹配。 | 请检查Format是否正确。 |
| static const int ACL_ERROR_BIN_SELECTOR_NOT_REGISTERED = 100028; | 使用二进制选择方式编译算子接口时，算子未注册选择器。 | 请检查是否调用aclopRegisterSelectKernelFunc接口注册算子选择器。 |
| static const int ACL_ERROR_KERNEL_NOT_FOUND = 100029; | 编译算子时，算子Kernel未注册。 | 请检查是否调用aclopCreateKernel接口注册算子Kernel。 |
| static const int ACL_ERROR_BIN_SELECTOR_ALREADY_REGISTERED = 100030; | 使用二进制选择方式编译算子接口时，算子重复注册。 | 请检查是否重复调用aclopRegisterSelectKernelFunc接口注册算子选择器。 |
| static const int ACL_ERROR_KERNEL_ALREADY_REGISTERED = 100031; | 编译算子时，算子Kernel重复注册。 | 请检查是否重复调用aclopCreateKernel接口注册算子Kernel。 |
| static const int ACL_ERROR_INVALID_QUEUE_ID = 100032; | 无效的队列ID。 | 请检查队列ID是否正确。 |
| static const int ACL_ERROR_REPEAT_SUBSCRIBE = 100033; | 重复订阅。 | 请检查针对同一个Stream，是否重复调用aclrtSubscribeReport接口。 |
| static const int ACL_ERROR_STREAM_NOT_SUBSCRIBE = 100034;<br>须知：此返回码后续版本会废弃，请使用ACL_ERROR_RT_STREAM_NO_CB_REG返回码。 | Stream未订阅。 | 请检查是否已调用aclrtSubscribeReport接口。 |
| static const int ACL_ERROR_THREAD_NOT_SUBSCRIBE = 100035;<br>须知：此返回码后续版本会废弃，请使用ACL_ERROR_RT_THREAD_SUBSCRIBE返回码。 | 线程未订阅。 | 请检查是否已调用aclrtSubscribeReport接口。 |
| static const int ACL_ERROR_WAIT_CALLBACK_TIMEOUT = 100036;<br>须知：此返回码后续版本会废弃，请使用ACL_ERROR_RT_REPORT_TIMEOUT返回码。 | 等待callback超时。 | 请检查是否已调用aclrtLaunchCallback接口下发callback任务；<br>请检查aclrtProcessReport接口中超时时间是否合理；<br>请检查callback任务是否已经处理完成，如果已处理完成，但还调用aclrtProcessReport接口，则需优化代码逻辑。 |
| static const int ACL_ERROR_REPEAT_FINALIZE = 100037; | 重复去初始化。 | 请检查是否重复调用aclFinalize接口或重复调用aclFinalizeReference接口进行去初始化。 |
| static const int ACL_ERROR_NOT_STATIC_AIPP = 100038;<br>须知：此返回码后续版本会废弃，请使用ACL_ERROR_GE_AIPP_NOT_EXIST返回码。 | 静态AIPP配置信息不存在。 | 调用aclmdlGetFirstAippInfo接口时，请传入正确的index值。 |
| static const int ACL_ERROR_COMPILING_STUB_MODE = 100039; | 运行应用前配置的动态库路径是编译桩的路径，不是正确的动态库路径。 | 请检查动态库路径的配置，确保使用运行模式的动态库。 |
| static const int ACL_ERROR_GROUP_NOT_SET = 100040;<br>须知：此返回码后续版本会废弃，请使用ACL_ERROR_RT_GROUP_NOT_SET返回码。 | 未设置Group。 | 请检查是否已调用aclrtSetGroup接口。 |
| static const int  ACL_ERROR_GROUP_NOT_CREATE = 100041;<br>须知：此返回码后续版本会废弃，请使用ACL_ERROR_RT_GROUP_NOT_CREATE返回码。 | 未创建对应的Group。 | 请检查调用接口时设置的Group ID是否在支持的范围内，Group ID的取值范围：[0, (Group数量-1)]，用户可调用aclrtGetGroupCount接口获取Group数量。 |
| static const int ACL_ERROR_PROF_ALREADY_RUN = 100042; | 已存在采集Profiling数据的任务。 | - 请检查代码逻辑，“通过调用AscendCL API方式采集Profiling数据”的配置不能与其它方式的Profiling配置并存，只能保留一种。<br>  - 请检查是否对同一个Device重复下发了多次Profiling配置。 |
| static const int ACL_ERROR_PROF_NOT_RUN = 100043; | 未使用aclprofInit接口先进行Profiling初始化。 | 请检查接口调用顺序。 |
| static const int ACL_ERROR_DUMP_ALREADY_RUN = 100044; | 已存在获取Dump数据的任务。 | 请检查在调用aclmdlInitDump接口、aclmdlSetDump接口、aclmdlFinalizeDump接口配置Dump信息前，是否已调用aclInit接口配置Dump信息，如是，请调整代码逻辑，保留一种方式配置Dump信息即可。 |
| static const int ACL_ERROR_DUMP_NOT_RUN = 100045; | 未使用aclmdlInitDump接口先进行Dump初始化。 | 请检查获取Dump数据的接口调用顺序，参考aclmdlInitDump接口处的说明。 |
| static const int ACL_ERROR_PROF_REPEAT_SUBSCRIBE = 148046; | 重复订阅同一个模型。 | 请检查接口调用顺序。 |
| static const int ACL_ERROR_PROF_API_CONFLICT = 148047; | 采集性能数据的接口调用冲突。 | 两种方式的Profiling性能数据采集接口不能交叉调用，aclprofInit接口和aclprofFinalize接口之间不能调用aclprofModelSub接口、aclprofGet*接口、aclprofModelUnSubscribe接口，aclprofModelSubscribe接口和aclprofModelUnSubscribe接口之间不能调用aclprofInit接口、aclprofStart接口、aclprofStop接口、aclprofFinalize。 |
| static const int ACL_ERROR_INVALID_MAX_OPQUEUE_NUM_CONFIG = 148048; | 无效的算子缓存信息老化配置。 | 请检查算子缓存信息老化配置，参考aclInit处的配置说明及示例。 |
| static const int ACL_ERROR_INVALID_OPP_PATH = 148049; | 没有设置ASCEND_OPP_PATH环境变量，或该环境变量的值设置错误。 | 请检查是否设置ASCEND_OPP_PATH环境变量，且该环境变量的值是否为opp软件包的安装路径。 |
| static const int ACL_ERROR_OP_UNSUPPORTED_DYNAMIC = 148050; | 算子不支持动态Shape。 | - 请检查单算子模型文件中该算子的Shape是否为动态，如果是动态的，需要修改为固定Shape。<br>  - 请检查编译算子时，aclTensorDesc的Shape是否为动态，如果是动态的，需要按照固定Shape重新创建aclTensorDesc。 |
| static const int ACL_ERROR_RELATIVE_RESOURCE_NOT_CLEARED = 148051; | 相关的资源尚未释放。 | 在销毁通道描述信息时，如果相关的通道尚未销毁则返回此错误码。请检查与此通道描述信息相关联的通道是否被销毁。 |
| static const int ACL_ERROR_UNSUPPORTED_JPEG = 148052; | JPEGD功能不支持的输入图片编码格式（例如算术编码、渐进式编码等）。 | 实现JPEGD图片解码功能时，仅支持Huffman编码，压缩前的原图像色彩空间为YUV，像素的各分量比例为4:4:4或4:2:2或4:2:0或4:0:0或4:4:0，不支持算术编码、不支持渐进JPEG格式、不支持JPEG2000格式。 |
| static const int ACL_ERROR_INVALID_BUNDLE_MODEL_ID = 148053; | 无效的模型ID。 | 请检查模型ID是否正确、模型是否正确加载。 |
| static const int ACL_ERROR_BAD_ALLOC = 200000; | 申请内存失败。 | 请检查硬件环境上的内存剩余情况。 |
| static const int ACL_ERROR_API_NOT_SUPPORT = 200001; | 接口不支持。 | 请检查调用的接口当前是否支持。 |
| static const int ACL_ERROR_INVALID_DEVICE = 200002;<br>须知：此返回码后续版本会废弃，请使用ACL_ERROR_RT_INVALID_DEVICEID返回码。 | 无效的Device。 | 请检查Device是否存在。 |
| static const int ACL_ERROR_MEMORY_ADDRESS_UNALIGNED = 200003; | 内存地址未对齐。 | 请检查内存地址是否符合接口要求。 |
| static const int ACL_ERROR_RESOURCE_NOT_MATCH = 200004; | 资源不匹配。 | 请检查调用接口时，是否传入正确的Stream、Context等资源。 |
| static const int ACL_ERROR_INVALID_RESOURCE_HANDLE = 200005; | 无效的资源句柄。 | 请检查调用接口时，传入的Stream、Context等资源是否已被销毁或占用。 |
| static const int ACL_ERROR_FEATURE_UNSUPPORTED = 200006; | 特性不支持。 | 您可以获取日志后提交ISSUE。 |
| static  const int ACL_ERROR_PROF_MODULES_UNSUPPORTED = 200007; | 下发了不支持的Profiling配置。 | 请参见aclprofCreateConfig中的说明检查Profiling的配置是否正确。 |
| static const int ACL_ERROR_STORAGE_OVER_LIMIT = 300000; | 超出存储上限。 | 请检查硬件环境上的存储剩余情况。 |
| static const int ACL_ERROR_INTERNAL_ERROR = 500000; | 未知内部错误。 | 您可以获取日志后提交ISSUE。 |
| static const int ACL_ERROR_FAILURE = 500001; | 内部错误。 | 您可以获取日志后提交ISSUE。 |
| static const int ACL_ERROR_GE_FAILURE = 500002; | GE（Graph Engine）模块的错误。 | 您可以获取日志后提交ISSUE。 |
| static const int ACL_ERROR_RT_FAILURE = 500003; | RUNTIME模块的错误。 | 您可以获取日志后提交ISSUE。 |
| static const int ACL_ERROR_DRV_FAILURE = 500004; | Driver模块的错误。 | 您可以获取日志后提交ISSUE。 |
| static const int ACL_ERROR_PROFILING_FAILURE = 500005; | Profiling模块的错误。 | 您可以获取日志后提交ISSUE。 |

**表 2**  透传GE的返回码列表

| 返回码 | 含义 | 可能原因及解决方法 |
| --- | --- | --- |
| uint32_t ACL_ERROR_GE_PARAM_INVALID = 145000; | 参数校验失败。 | 请检查接口的入参值是否正确。 |
| uint32_t ACL_ERROR_GE_EXEC_NOT_INIT = 145001; | 未初始化。 | - 请检查是否已调用aclInit接口进行初始化，请确保已调用aclInit接口，且在其它acl接口之前调用。<br>  - 请检查是否已调用对应功能的初始化接口，例如初始化Dump的aclmdlInitDump接口、初始化Profiling的aclprofInit接口。 |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID = 145002; | 无效的模型路径。 | 请检查模型路径是否正确。 |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_ID_INVALID = 145003; | 无效的模型ID。 | 请检查模型ID是否正确、模型是否正确加载。 |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID = 145006; | 无效的模型大小。 | 模型文件无效，请重新构建模型。 |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID = 145007; | 无效的模型内存地址。 | 请检查模型地址是否有效。 |
| uint32_t ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID = 145008; | 无效的队列ID。 | 请检查队列ID是否正确。 |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_MODEL_REPEATED = 145009; | 重复初始化或重复加载。 | 请检查是否调用对应的接口重复初始化或重复加载。 |
| uint32_t ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID = 145011; | 无效的动态分档输入地址。 | 请检查动态分档输入地址。 |
| uint32_t ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID = 145012; | 无效的动态分档输入长度。 | 请检查动态分档输入长度。 |
| uint32_t ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID = 145013; | 无效的动态分档Batch大小。 | 请检查动态分档Batch大小。 |
| uint32_t ACL_ERROR_GE_AIPP_BATCH_EMPTY = 145014; | 无效的AIPP batch size。 | 请检查AIPP batch size是否正确。 |
| uint32_t ACL_ERROR_GE_AIPP_NOT_EXIST = 145015; | AIPP配置不存在。 | 请检查AIPP是否配置。 |
| uint32_t ACL_ERROR_GE_AIPP_MODE_INVALID = 145016; | 无效的AIPP模式。 | 请检查模型转换时配置的AIPP模式是否正确。 |
| uint32_t ACL_ERROR_GE_OP_TASK_TYPE_INVALID = 145017; | 无效的任务类型。 | 请检查算子类型是否正确。 |
| uint32_t ACL_ERROR_GE_OP_KERNEL_TYPE_INVALID = 145018; | 无效的算子类型。 | 请检查算子类型是否正确。 |
| uint32_t ACL_ERROR_GE_PLGMGR_PATH_INVALID = 145019; | 无效的so文件，包括so文件的路径层级太深、so文件被误删除等情况。 | 请检查运行应用前配置的环境变量LD_LIBRARY_PATH是否正确，详细描述请参见编译运行处的操作指导。 |
| uint32_t ACL_ERROR_GE_FORMAT_INVALID = 145020; | 无效的format。 | 请检查Tensor数据的format是否有效。 |
| uint32_t ACL_ERROR_GE_SHAPE_INVALID = 145021; | 无效的shape。 | 请检查Tensor数据的shape是否有效。 |
| uint32_t ACL_ERROR_GE_DATATYPE_INVALID = 145022; | 无效的数据类型。 | 请检查Tensor数据的数据类型是否有效。 |
| uint32_t ACL_ERROR_GE_MEMORY_ALLOCATION = 245000; | 申请内存失败。 | 请检查硬件环境上的内存剩余情况。 |
| uint32_t ACL_ERROR_GE_MEMORY_OPERATE_FAILED = 245001; | 内存初始化、内存复制操作失败。 | 请检查内存地址是否正确、硬件环境上的内存是否足够等。 |
| uint32_t ACL_ERROR_GE_DEVICE_MEMORY_ALLOCATION_FAILED = 245002; | 申请Device内存失败。 | Device内存已用完，无法继续申请，请释放部分Device内存，再重新尝试。 |
| uint32_t  ACL_ERROR_GE_SUBHEALTHY = 345102; | 亚健康状态。 | 设备或进程异常触发的重部署动作完成后的状态为亚健康状态，亚健康状态下可以正常调用相关接口。 |
| static const uint32_t ACL_ERROR_GE_USER_RAISE_EXCEPTION = 345103; | 用户自定义函数主动抛异常。 | 用户可根据DataFlowInfo中设置的UserData识别出来哪个输入的数据执行报错了，再根据报错排查问题。 |
| static const uint32_t ACL_ERROR_GE_DATA_NOT_ALIGNED = 345104; | 数据未对齐。 | 若用户自定义函数存在多个输出时，需排查用户代码中是否少设置输出，缺少输出可能会导致数据对齐异常。 |
| uint32_t ACL_ERROR_GE_INTERNAL_ERROR = 545000; | 未知内部错误。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_LOAD_MODEL = 545001; | 系统内部加载模型失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_MODEL_PARTITION_FAILED = 545002; | 系统内部加载模型失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_WEIGHT_PARTITION_FAILED = 545003; | 系统内部加载模型权值失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_TASK_PARTITION_FAILED = 545004; | 系统内部加载模型任务失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_EXEC_LOAD_KERNEL_PARTITION_FAILED = 545005; | 系统内部加载模型算子失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_EXEC_RELEASE_MODEL_DATA = 545006; | 系统内释放模型空间失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_COMMAND_HANDLE = 545007; | 系统内命令操作失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_GET_TENSOR_INFO = 545008; | 系统内获取张量数据失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_UNLOAD_MODEL = 545009; | 系统内卸载模型空间失败。 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_MODEL_EXECUTE_TIMEOUT = 545601; | 模型执行超时 | 您可以获取日志后提交ISSUE。 |
| uint32_t ACL_ERROR_GE_REDEPLOYING = 545602; | 正在重部署。 | 等待重部署动作完成后重新调用相关接口。 |
