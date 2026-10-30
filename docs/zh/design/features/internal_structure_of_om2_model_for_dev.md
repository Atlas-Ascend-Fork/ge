# OM2 模型包内部结构

本文面向需要查看、解析 OM2 模型包的开发者，统一说明包内文件清单和 JSON 字段。

## 1. 包结构文件清单

OM2 为 ZIP 归档。以下路径均相对包内根前缀 `<base_name>/`：在线构图时通常为图名，ATC 保存时为输出文件名。`N` 是从 0 开始的模型编号，单模型使用 `model_0`，Bundle 按模型数量递增。

第二列说明文件在哪些模型场景下出现。“所有模型”指正常构建的静态模型、动态分档模型，以及带 AIPP 的上述模型。AIPP 是输入预处理能力，可与静态或动态分档模型组合，并非与它们互斥的第三类模型。权重、变量、自定义算子等文件是否出现，还取决于模型实际包含的数据和编译产物。

| 包内文件/目录 | 出现条件/适用场景 | 说明 |
|---------------|-------------------|------|
| `manifest.json` | 所有模型 | 包清单，记录版本信息、模型数量和 ATC 命令 |
| `data/model_N/` | 所有模型 | 每个子模型的数据目录；数量由 `manifest.json` 的 `model_num` 指定 |
| `data/model_N/model_meta.json` | 所有模型 | 模型元信息，包括模型名称、工作内存、输入输出 Tensor 描述，以及动态分档和 AIPP 配置，详见第 2.2 节 |
| `data/model_N/op_attr.json` | 所有模型 | 算子属性，当前记录算子对应的原始算子名称列表 |
| `data/model_N/constants_config.json` | 所有模型 | 常量配置，包括内置权重总大小、各常量的数据来源、文件名、偏移和大小 |
| `data/model_N/variables_config.json` | 变量配置对象存在，且 `var_metas` 或 `entries` 非空时 | 变量配置，包括图 ID、变量元信息、变量资源、转换路径及初始化数据的位置 |
| `data/model_N/runtime/lib<model>_om2.so` | 所有模型 | 编译后的模型动态库，包含模型加载、执行与资源释放逻辑，导出 `GertModel*` C 接口 |
| `data/model_N/runtime/csrc/` | 所有模型（固定打包源码） | 模型专属 C++ 源码、接口头文件和重编译构建文件；打包器固定写入，但非模型加载必需（加载仅读取运行时 `.so`） |
| `data/model_N/runtime/csrc/om2_model_api.h` | 所有模型（固定打包） | 运行时 C API 头文件副本，定义见头文件参考 |
| `data/model_N/runtime/csrc/<model>_internal.h` | 所有模型（固定打包） | 模型内部接口声明 |
| `data/model_N/runtime/csrc/<model>_resources.cpp` | 所有模型（固定打包） | 运行时资源初始化与释放逻辑 |
| `data/model_N/runtime/csrc/<model>_args_manager.cpp` | 所有模型（固定打包） | 模型参数表管理逻辑 |
| `data/model_N/runtime/csrc/<model>_kernel_reg.cpp` | 所有模型（固定打包） | 算子二进制注册逻辑 |
| `data/model_N/runtime/csrc/<model>_load_and_run.cpp` | 所有模型（固定打包） | 模型加载执行逻辑 |
| `data/model_N/runtime/csrc/Makefile` | 所有模型（固定打包） | 重编译构建入口 |
| `data/model_N/debug/ge_visual_00000000_graph_0.json` | 模型的调试对象存在时 | 模型图的可视化 JSON，包含图、算子、连接关系和属性等调试信息 |
| `data/constants/constant_N` | 模型的 `internal_weight_size > 0` 且有内置权重数据时；纯外置权重场景不生成 | 内置常量和权重的二进制数据，由常量配置中的文件名、偏移和大小关联 |
| `data/variables/var_weight_data_N` | 变量包含非空初始化数据时 | 按变量条目拼接的初始化二进制数据，由变量配置中的偏移和大小定位 |
| `data/kernels/` | 模型包含算子二进制时 | 预编译的算子二进制，如 `te_{opType}_{hashId}.o`，可供包内多个模型引用 |
| `data/custom_ops/binaries_npu_arch/` | 模型包含自定义算子二进制时 | 存放相应自定义算子二进制 |
| `data/custom_ops/shared_libs/` | 模型包含自定义算子共享库时 | 存放相应自定义算子共享库 |

## 2. JSON 文件字段说明

下列每个小节对应一个 JSON 文件，嵌套字段用点号表示，`[]` 表示数组元素，`<key>` 表示动态对象键。类型是 JSON 类型，不是 C++ 内存布局；整数的位宽、单位及写入条件在说明中标注。`struct_size`、`reserved` 和进程内指针不作为 JSON 字段落盘。

“使用”按当前代码实现说明：ACL 接口通常不直接读取 JSON，而是经反序列化、执行器或 `aclmdlDesc` 间接使用字段；表内使用 ACL 公开接口名，对应 OM2 实现函数通常带 `ImplOm2` 后缀。内部接口使用类名或函数名标明。仅解析保存、用于诊断或未被当前消费路径使用的字段会单独注明，不能据字段名推断存在对应 ACL 能力。

### 2.1 manifest.json

| JSON 字段 | 类型 | 必选 | 说明 |
|-----------|------|------|------|
| `compatibility` | object | 是 | 版本与特性信息对象；`DeserializeManifest` 解析后交给 `ValidateVersionCompatibility` 校验，供 ACL 模型加载及元信息查询路径共用 |
| `compatibility.compiler_version` | string | 是 | 编译侧版本；`ValidateVersionCompatibility` 比较编译器与当前执行器的主版本，编译侧主版本更高时拒绝解析 |
| `compatibility.required_executor_version` | string | 是 | 模型所需执行器版本；`ValidateVersionCompatibility` 在该值非空时比较所需版本与当前执行器版本，所需版本更高时拒绝解析 |
| `compatibility.used_features` | object | 是 | 使用特性名到版本串的映射，无使用特性时为空对象；`BuildUsedFeaturesStr` 将其用于版本不兼容的错误诊断，当前不逐特性校验版本 |
| `compatibility.used_features.<feature_name>` | string | 否（使用对应特性时写入） | 对应特性的版本串；由 `BuildUsedFeaturesStr` 输出到兼容性诊断信息，无独立 ACL 查询接口 |
| `model_num` | integer | 是 | uint64 语义的模型数量；单模型为 1，Bundle 为子模型数量；`PrepareModelUnit` 用于子模型下标校验，`GetOm2BundleInfo` 用于枚举子模型，间接服务 `aclmdlBundleGetModelNum`、`aclmdlBundleGetQueryModelNum` |
| `atc_command` | string | 是 | ATC 命令记录，非 ATC 路径可为空串；`DeserializeManifest` 解析保存，当前 OM2 ACL 加载和查询实现未读取该成员返回给用户 |

表中“必选”列按打包器行为标注：“必选=是”表示打包器恒写该字段。反序列化侧仅强制校验 `compatibility` 和 `model_num` 两个顶层字段，其余字段缺失时按宽容策略跳过，不会导致解析失败。

### 2.2 model_meta.json

对应每个 `data/model_N/model_meta.json`。编译期生成，加载期解析为 `GertModelDataModelMeta`，服务 ACL 模型描述和查询接口。模型 SO 的资源需求已固化在生成代码中。缺失本文件或模型名称会导致反序列化失败。

| JSON 字段 | 类型 | 必选 | 说明 |
|-----------|------|------|------|
| `name` | string | 是 | 模型名称，对应内存成员 `model_name`；`Om2ModelExecutor::Impl::InitModelDumpInfo` 用于填充 dump/profiling 模型信息，不是输入输出 Tensor 名称查询的来源 |
| `work_size` | integer | 是 | uint64 语义的工作内存字节数；`aclmdlQuerySize`、`aclmdlQuerySizeFromMem`、默认模式的 `aclmdlQueryWorkspaceSize` / `aclmdlQueryWorkspaceSizeFromMem` 返回此值，`aclmdlBundleGetSize` 返回对应子模型的此值；加载时 `CheckExternalWorkSize` 用于外部工作内存校验 |
| `zero_copy_size` | integer | 是 | int64 语义的输入输出内存优化可扣减字节数；`aclmdlQueryWorkspaceSize` / `aclmdlQueryWorkspaceSizeFromMem` 在 `ACL_WORKSPACE_MEM_OPTIMIZE_INPUTOUTPUT` 模式返回 `work_size - zero_copy_size`；加载时 `CheckExternalWorkSize` 固定按 `work_size - zero_copy_size` 校验外部工作内存 |
| `inputs` | `array<object>` | 是 | 输入 Tensor 描述数组；经 `aclmdlGetDesc` 或 `aclmdlGetDescFromFile` / `aclmdlGetDescFromMem` 填入描述对象，供 `aclmdlGetNumInputs` 及各输入查询接口使用；执行时也用于检查输入数量 |
| `outputs` | `array<object>` | 是 | 输出 Tensor 描述数组；经模型描述获取接口填入描述对象，供 `aclmdlGetNumOutputs` 及各输出查询接口使用；执行时也用于检查输出数量 |
| `inputs[].name` / `outputs[].name` | string | 是 | Tensor 名称；分别供 `aclmdlGetInputNameByIndex` / `aclmdlGetOutputNameByIndex`、`aclmdlGetInputIndexByName` / `aclmdlGetOutputIndexByName` 使用，也用于 `aclmdlGetTensorRealName`；动态分档输入名还用于 `aclmdlSetInputDynamicDims` 匹配原始 shape |
| `inputs[].index` / `outputs[].index` | integer | 是 | 数组下标，按序列化位置写入；当前 `ParseModelMetaInputs` / `ParseModelMetaOutputs` 不读取该字段，ACL 查询下标采用数组实际顺序 |
| `inputs[].shape` | `array<integer>` | 是 | 静态模型的输入 shape，供 `aclmdlGetInputDims` 使用；动态分档模型为原始输入 shape，可含 -1，解析到 `origin_input_dims` 后供 `aclmdlSetInputDynamicDims` 识别动态轴，此时普通输入描述由 `max_gear_shape` 覆盖 |
| `outputs[].shape` | `array<integer>` | 是 | 输出 shape；供 `aclmdlGetOutputDims` 使用，`aclmdlGetCurOutputDims` 在未设置当前档位时回退到此值；`PrepareOm2Tensor` 也用其构造执行 Tensor |
| `inputs[].max_gear_shape` | `array<integer>` | 否（仅动态分档模型写入） | 最大档位输入 shape，解析后覆盖 `input_desc.shape`，供 `aclmdlGetInputDims` 和 `PrepareOm2Tensor` 使用；未提供专用 V2 shape 时也作为 V2 描述的初始值 |
| `inputs[].shape_aclmdlGetInputDimsV2` | `array<integer>` | 否（`aipp_infos` 非空时写入） | `aclmdlGetInputDimsV2` 语义下的输入 shape；无对应 V2 描述时取该输入原描述；`ParseModelMetaInputs` 将其写入 `input_desc_v2.shape`，最终由 `aclmdlGetInputDimsV2` 返回 |
| `inputs[].data_type` / `outputs[].data_type` | integer | 是 | `ge::DataType` 枚举值，如 `DT_FLOAT=0`；分别供 `aclmdlGetInputDataType` / `aclmdlGetOutputDataType` 使用，也由 `PrepareOm2Tensor` 设置执行 Tensor 类型；动态范围大小计算还会使用数据类型字节数 |
| `inputs[].format` / `outputs[].format` | integer | 是 | `ge::Format` 枚举值，如 `FORMAT_NCHW=0`；分别供 `aclmdlGetInputFormat` / `aclmdlGetOutputFormat` 使用，也由 `PrepareOm2Tensor` 设置执行 Tensor 格式 |
| `inputs[].size` / `outputs[].size` | integer | 是 | uint64 语义的 Tensor 字节数；分别作为 `aclmdlGetInputSizeByIndex` / `aclmdlGetOutputSizeByIndex` 的返回来源；shape 含负值且范围非空时，接口改按最大范围计算大小 |
| `inputs[].shape_range` / `outputs[].shape_range` | `array<array<integer>>` | 是 | 每维为 `[min,max]`，静态模型可为空数组；输入范围供 `aclmdlGetInputDimsRange` 使用，输入和输出范围分别参与 `aclmdlGetInputSizeByIndex` / `aclmdlGetOutputSizeByIndex` 的最大大小计算；输入范围还用于动态 AIPP 校验分支判断 |
| `dynamic_dims` | object | 否（`dynamic_batch_info` 非空时写入） | `ParseModelMetaDynamicDims` 解析后，供 `aclmdlGetDesc` 填充档位信息及动态档位设置接口校验使用 |
| `dynamic_dims.dynamic_type` | integer | 是 | 动态分档类型，取值与 GE 动态分档类型枚举一致；`GetDynamicTensorInfoHelp` 按类型填充 batch、HW 或 dims 档位，`Om2ModelExecutor::SetDynamicSize` 校验 `aclmdlSetDynamicBatchSize`、`aclmdlSetDynamicHWSize`、`aclmdlSetInputDynamicDims` 请求的类型 |
| `dynamic_dims.user_designate_shape_order` | `array<string>` | 是 | 用户指定的 shape 信息顺序；`aclmdlGetDesc` 保存到 `dataNameOrder`，供 `aclmdlGetInputDynamicDims` 还原各档位输入维度；`aclmdlSetInputDynamicDims` 按此顺序拼接原始 shape 并提取动态轴 |
| `dynamic_dims.gears` | `array<object>` | 是 | 档位数组；数组长度供 `aclmdlGetInputDynamicGearCount` 等档位查询使用，也由 `Om2ModelExecutor::GetBatchInfoSize` 为 `aclmdlGetFirstAippInfo` 提供 `shapeCount` |
| `dynamic_dims.gears[].inputs` | `array<integer>` | 是 | 该档位各动态维度取值，对应 `dynamic_batch_info` 的一行；供 `aclmdlGetDynamicBatch`、`aclmdlGetDynamicHW`、`aclmdlGetInputDynamicDims` 查询，并由 `SetDynamicSize` 校验档位设置请求；`aclmdlGetCurOutputDims` 也用其定位当前档位 |
| `dynamic_dims.gears[].outputs` | `array<array<integer>>` | 是 | 该档位的各输出 shape；由内存中逗号分隔串按档位与输出信息转换，维度从第三项开始取值；经 `GetModelAttrs`、`GetModelOutputShapeInfoHelp` 填入描述对象，供 `aclmdlGetCurOutputDims` 返回当前档位输出维度 |
| `aipp` | object | 否（`aipp_infos` 非空时写入） | 数组仅保留非空且 `aipp_type != DATA_WITHOUT_AIPP` 的条目；`ParseAippJson` 解析后供 AIPP 查询和设置时的模型约束校验使用 |
| `aipp.aipp_infos` | `array<object>` | 是 | AIPP 输入配置数组；`Om2ModelExecutor` 保存后供 `aclmdlGetAippType`、`aclmdlGetFirstAippInfo`、`aclmdlSetInputAIPP`、`aclmdlSetAIPPByInputIndex` 间接使用 |
| `aipp.aipp_infos[].index` | integer | 是 | 原 AIPP 数组中的下标；`ParseAippJson` 按此值恢复稀疏配置位置，使 `GetAippInfo`、`GetAippType` 等可按 ACL 输入下标查找 |
| `aipp.aipp_infos[].aipp_type` | integer | 是 | `ge::InputAippType` 枚举值；`aclmdlGetAippType` 返回类型，`aclmdlSetInputAIPP` / `aclmdlSetAIPPByInputIndex` 用于检查动态 AIPP 输入关联；执行器其他 AIPP 查询也据此判断是否存在配置 |
| `aipp.aipp_infos[].aipp_data_index` | integer | 是 | 关联输入下标；无关联时为 `kOm2InvalidAippDataIndex`（0xFFFFFFFF）；`aclmdlGetAippType` 对动态 AIPP 返回该关联下标，`aclmdlSetAIPPByInputIndex` 用其定位参数输入 |
| `aipp.aipp_infos[].aipp_mode` | integer | 是 | AIPP 模式；`ParseAippConfigFromJson` 解析到 `AippConfigInfo`，可经内部 `GetAippInfo` 取得；当前 `SetAippInfo` 未将其映射到 ACL 查询结果，动态 AIPP 校验也未读取该成员 |
| `aipp.aipp_infos[].input_format` | integer | 是 | AIPP 输入格式；经 `GetAippInfo`、`SetAippInfo` 供 `aclmdlGetFirstAippInfo` 返回 `inputFormat` |
| `aipp.aipp_infos[].src_image_size_w` / `src_image_size_h` | integer | 是 | 源图宽、高；本表后续省略前缀的 AIPP 字段均属于同一数组元素；`aclmdlGetFirstAippInfo` 分别返回为 `srcImageSizeW` / `srcImageSizeH` |
| `crop` / `resize` / `padding` / `csc_switch` | integer | 是 | 裁剪、缩放、填充、色度转换开关，0 关闭，非 0 开启；`aclmdlGetFirstAippInfo` 经 `SetAippInfo` 返回对应开关 |
| `load_start_pos_w` / `load_start_pos_h` | integer | 是 | 裁剪起始位置；`aclmdlGetFirstAippInfo` 返回为 `loadStartPosW` / `loadStartPosH` |
| `crop_size_w` / `crop_size_h` | integer | 是 | 裁剪输出宽、高；`aclmdlGetFirstAippInfo` 返回为 `cropSizeW` / `cropSizeH` |
| `resize_output_w` / `resize_output_h` | integer | 是 | 缩放输出宽、高；`aclmdlGetFirstAippInfo` 返回为 `resizeOutputW` / `resizeOutputH` |
| `left_padding_size` / `right_padding_size` / `top_padding_size` / `bottom_padding_size` | integer | 是 | 四边填充量；`aclmdlGetFirstAippInfo` 经 `SetAippInfo` 返回对应填充量 |
| `matrix_r0c0` ~ `matrix_r2c2` | integer | 是 | 3×3 CSC 矩阵系数，共 9 个字段；`aclmdlGetFirstAippInfo` 返回为 `matrixR0C0` ~ `matrixR2C2` |
| `output_bias_0` ~ `output_bias_2` / `input_bias_0` ~ `input_bias_2` | integer | 是 | CSC 输出、输入偏置，共 6 个字段；`aclmdlGetFirstAippInfo` 返回为 `outputBias0` ~ `outputBias2` / `inputBias0` ~ `inputBias2` |
| `rbuv_swap_switch` / `ax_swap_switch` / `single_line_mode` | integer | 是 | 通道交换及单行模式配置；`aclmdlGetFirstAippInfo` 返回为 `rbuvSwapSwitch` / `axSwapSwitch` / `singleLineMode` |
| `mean_chn_0` ~ `mean_chn_3` | integer | 是 | 四通道均值；`aclmdlGetFirstAippInfo` 返回为 `meanChn0` ~ `meanChn3` |
| `min_chn_0` ~ `min_chn_3` | number | 是 | 四通道最小值；`aclmdlGetFirstAippInfo` 返回为 `minChn0` ~ `minChn3` |
| `var_reci_chn_0` ~ `var_reci_chn_3` | number | 是 | 四通道方差倒数；`aclmdlGetFirstAippInfo` 返回为 `varReciChn0` ~ `varReciChn3` |
| `support_rotation` | integer | 是 | 旋转支持开关；解析保存到 `AippConfigInfo`，可经内部 `GetAippInfo` 取得；当前 `SetAippInfo` 未映射到 ACL 查询结果，动态 AIPP 校验也未读取该成员 |
| `related_input_rank` | integer | 是 | 关联输入的序号（rank）；`aclmdlSetInputAIPP` 的 `GetAndCheckAippParamsOm2` 用其定位原始输入，校验 AIPP 输出的 batch、宽、高；`aclmdlSetAIPPByInputIndex` 间接复用此校验 |
| `max_src_image_size` | integer | 是 | 最大源图大小；`aclmdlSetInputAIPP` 的 `GetAndCheckAippParamsOm2` 用于校验用户动态 AIPP 源图字节数上限，`aclmdlSetAIPPByInputIndex` 间接复用此校验 |
| `aipp_inputs` / `aipp_outputs` | `array<string>` | 是 | AIPP 前/后输入输出描述；每项格式为 `format:data_type:name:size:dim_num:dim0,dim1,...`；经 `GetAllAippInputOutputDims` 供 `aclmdlGetFirstAippInfo` 返回各档位的维度和字节数；`aclmdlSetInputAIPP` 还用输入维度校验输出 shape，字符串中的 format/data_type 部分不由 `ParseAippDimInfo` 读取 |
| `orig_input_format` | integer | 是 | 原始输入格式；经 `GetOrigInputInfo` 供 `aclmdlGetFirstAippInfo` 返回 `srcFormat`，并用于 `aclmdlSetInputAIPP` 校验时识别 NCHW/NHWC 的 N、H、W 位置 |
| `orig_input_data_type` | integer | 是 | 原始输入数据类型；经 `GetOrigInputInfo` 供 `aclmdlGetFirstAippInfo` 返回 `srcDatatype` |
| `orig_input_dim_num` | integer | 是 | 原始输入维数；经 `GetOrigInputInfo` 供 `aclmdlGetFirstAippInfo` 返回 `srcDimNum` |

### 2.3 op_attr.json

当前 `Om2PackageHelper::BuildDebugInfo` 只收集非空的原始算子名列表，按算子名称组织。属性键由 `ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES` 定义，不应假定包含算子的全部属性；无匹配属性时为 `{}`。

| JSON 字段 | 类型 | 必选 | 说明 |
|-----------|------|------|------|
| `<op_name>` | object | 否（匹配到非空原始算子名列表时写入） | 算子名对应的属性集合；`ParseOpAttrJsonToMapInternal` 按算子名组织属性，`aclmdlGetDesc` 缓存后供 `aclmdlGetOpAttr` 按 `opName` 查询 |
| `<op_name>.<attr_name>` | object | 是 | 属性键为 `ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES` 对应字符串；`aclmdlGetOpAttr` 按 `attr` 参数匹配此键 |
| `<op_name>.<attr_name>.type` | string | 是 | 当前固定为 `LIST_STRING`；`ParseOpAttrJsonToMapInternal` 据此选择字符串列表编码，作为 `aclmdlGetOpAttr` 返回值的格式 |
| `<op_name>.<attr_name>.value` | `array<string>` | 是 | 编译前的原始算子名称列表，用于 dump 等关联；`ParseOpAttrJsonToMapInternal` 将各项编码为 `[字符串长度]字符串` 并拼接，供 `aclmdlGetOpAttr` 返回，不直接返回 JSON 数组 |

### 2.4 constants_config.json

| JSON 字段 | 类型 | 必选 | 说明 |
|-----------|------|------|------|
| `internal_weight_size` | integer | 是 | uint64 语义的内置权重总字节数，无内置权重时为 0；供 `aclmdlQuerySize` / `aclmdlQuerySizeFromMem` 返回权重大小、`aclmdlBundleGetSize` 返回子模型权重大小；加载时 `PrepareInternalConsts` 用于分配或校验内置权重内存 |
| `consts` | object | 是 | 常量配置映射，可为空对象；不是数组；经 `DeserializeConstantsConfig`、`PrepareConstantsFromStruct` 为 ACL 模型加载准备常量地址表，传入 `GertModelLoad` |
| `consts.<key>` | object | 否（存在对应常量时写入） | `INTERNAL` 常量键为 `constant_<index>`，其他来源以算子名为键；`DeserializeConstantsConfig` 遍历对象值恢复常量条目，当前运行时地址表按条目内 `index` 而非对象键索引 |
| `consts.<key>.index` | integer | 是 | uint64 语义的常量下标；`PrepareConstantsFromStruct` 及各类 `Prepare*Consts` 用于确定 `GertModelLoadConfig.constants` 地址表长度和槽位 |
| `consts.<key>.type` | string | 是 | 常量来源类型；`ClassifyConstItems` 将 `INTERNAL`、`COMBINED`、`INDIVIDUAL` 分流到内置权重、合并外置权重、独立外置权重准备接口，未知类型导致加载失败 |
| `consts.<key>.file_name` | string | 是 | 数据源文件名；`FindInternalWeightBuf` 用于匹配包内常量文件，`PrepareCombinedConsts` / `PrepareIndividualConsts` 用于匹配外置权重文件或用户提供的文件常量内存；关联 `aclmdlSetExternalWeightAddress` 配置 |
| `consts.<key>.file_path` | string | 否（非离线序列化、非 INTERNAL 且路径非空时写入） | `RepackOm2ModelData` 的 `RewriteOm2ConstantsConfig` 在模型保存时用于外置权重搬迁并移除此字段；执行器 `ParseConstItems` 不读取该成员，不作为 ACL 离线加载的路径来源 |
| `consts.<key>.offset` | integer | 是 | int64 语义的数据源内字节偏移；各类 `Prepare*Consts` 用于定位内置权重地址、外置权重切片或用户文件常量内存中的数据 |
| `consts.<key>.size` | integer | 是 | int64 语义的常量字节数；各类 `Prepare*Consts` 用于权重边界和用户内存大小校验，以及外置权重读取、分配和拷贝 |

### 2.5 variables_config.json

配置对象存在且 `var_metas` 或 `entries` 非空时生成。变量初始化数据存入 `data/variables/var_weight_data_N`，不直接写入 JSON。

除单独注明的查询用途外，下表中的运行时变量接口由 `Om2ModelExecutor::Impl::PrepareVariablesFromStruct` / `PrepareVarAddrs` 在 ACL 单模型或 Bundle 子模型加载期间调用，最终将变量地址传给 `GertModelLoadConfig.var_addrs`；不是逐字段对外查询接口。

| JSON 字段 | 类型 | 必选 | 说明 |
|-----------|------|------|------|
| `graph_id` | integer | 是 | uint64 语义的图 ID；`PrepareVariablesFromStruct` 传入 `Om2RTVarManager::TransAllVarData`，用于筛选本图需要转换的变量 |
| `global_shared_var_size` | integer | 否（大于 0 时写入） | 全局共享变量 HBM 字节数；`GetOm2BundleInfo` 取各子模型最大值，供 `aclmdlBundleGetVarWeightSize` 返回，并供 `aclmdlBundleInitFromFile` / `aclmdlBundleInitFromMem` 校验外部变量内存大小 |
| `var_metas` | `array<object>` | 是 | 变量元信息，可为空数组；`ValidateVarMetas` 校验下标，`PrepareVarAddrs` 构造 SO 变量地址表，`PrepareVariablesFromStruct` 收集参与转换、拷贝的变量名 |
| `var_metas[].index` | integer | 是 | 变量下标；`ValidateVarMetas` 检查越界和重复，`PrepareVarAddrs` 将设备地址写入此下标对应的地址表槽位 |
| `var_metas[].var_name` | string | 是 | 变量名；`PrepareVarAddrs` 经 `Om2RTVarManager::GetVarDevAddr` 查找设备地址，也由 `TransAllVarData` / `CopyVarData` 定位变量 |
| `var_metas[].op_type` | string | 是 | 算子类型；`ParseVarMetaFromJson` 解析保存，当前 `PrepareVarAddrs` 和变量初始化流程未按该字段分支 |
| `var_metas[].op_name` | string | 是 | 算子名；`ParseVarMetaFromJson` 解析保存，当前变量地址准备和转换、拷贝流程不使用该成员，无对应 ACL 查询接口 |
| `var_metas[].tensor_desc` | object | 是 | Tensor 描述，字段见本表末尾；`ParseVarMetaFromJson` 解析保存，当前 `PrepareVarAddrs` 按变量名取地址，不使用此描述 |
| `entries` | object | 否（entries 非空时写入） | 变量资源映射；`DeserializeVariablesData` 恢复条目和初始化数据，`Om2RTVarManager::Init` 注册并合并共享变量资源 |
| `entries.<var_key>` | object | 是 | 以变量键索引的资源条目；`DeserializeVariablesData` 遍历对象值，但不使用外层键；`RTVarAddEntry` 和 `Om2RTVarManager::Init` 使用条目内 `var_key` |
| `entries.<var_key>.var_name` | string | 是 | 变量名；`RTVarResource::AddEntry` 建立名称到变量键的映射，供 `GetEntryByName`、`GetVarDevAddr`、变量转换和拷贝使用 |
| `entries.<var_key>.file_name` | string | 是 | 初始化数据文件基名，如 `var_weight_data_0`；`DeserializeVariablesData` 用第一条 entry 的该值定位 `data/variables/` 下的共享初始化数据文件，各条目随后从该文件切片 |
| `entries.<var_key>.var_key` | string | 是 | 变量键，与对象键对应；`RTVarAddEntry` 检查非空，`RTVarResource::AddEntry` 用作资源索引，`Om2RTVarManager::Init` 用于 Bundle 共享变量去重和一致性校验 |
| `entries.<var_key>.op_type` | string | 是 | 算子类型；`ParseVarEntryFromJson` 解析保存，`CopyVarEntry` 保留该值；当前 `Om2RTVarManager` 不按该字段决定分配、转换或拷贝逻辑 |
| `entries.<var_key>.logic_addr` | integer | 是 | uint64 语义的逻辑地址；`Om2RTVarManager::GetVarDevAddr` 在使用外部变量内存时计算相对变量基址的偏移，`Init` 也用于共享变量一致性校验 |
| `entries.<var_key>.size` | integer | 是 | uint64 语义的变量字节数；`AllocDevAddr` 用于设备内存分配，`GetVarDevAddr` 用于外部内存边界和初始化大小校验，`CopyVarFromDevice` / `CopyVarToDevice` 用于变量数据拷贝 |
| `entries.<var_key>.memory_type` | integer | 是 | 内存类型；`Om2RTVarManager::AllocDevAddr` 转为 `rtMemType_t` 后传给 `Om2Malloc`，`Init` 也用于共享变量一致性校验 |
| `entries.<var_key>.changed_graph_id` | integer | 是 | 变量变更关联图 ID；`TransAllVarData` 仅在该值等于当前 `graph_id` 且不等于 `allocated_graph_id` 时进一步检查转换条件 |
| `entries.<var_key>.allocated_graph_id` | integer | 是 | 变量分配关联图 ID；`TransAllVarData` 与 `changed_graph_id` 比较，判断是否需要跨图变量转换 |
| `entries.<var_key>.tensor_desc` | object | 是 | 变量 Tensor 描述；`Om2RTVarManager::Init` 用 shape、类型、格式校验共享变量一致性，`CopyVarData` 用源、目标类型判断是否需要转换，并用源 shape 计算元素数 |
| `entries.<var_key>.trans_road` | `array<object>` | 是 | 变量转换节点序列，可为空数组；`TransAllVarData` 筛选后由 `TransSingleVarData`、`TransVarOnHost` 执行主机侧格式或类型转换 |
| `entries.<var_key>.trans_road[].node_type` | string | 是 | 转换节点类型；`NeedRealTrans` / `TransVarOnHost` 用于跳过无需转换的节点，或选择 `TransDataFormat`、`TransTensorDataType` 等转换逻辑 |
| `entries.<var_key>.trans_road[].input` / `output` | object | 是 | 转换前、后 Tensor 描述；`TransSingleVarData` 用首节点输入描述构造旧变量键，`TransVarOnHost` 用 shape、格式和类型构造转换参数 |
| `entries.<var_key>.copy_info` | object | 是 | 变量拷贝信息；`Om2RTVarManager::CopyVarData` 用于恢复变量间的数据拷贝关系 |
| `entries.<var_key>.copy_info.src_var_name` | string | 是 | 源变量名；`CopyVarData` 与源描述共同传入 `RTVarResource::BuildVarKey`，用于查找源变量 |
| `entries.<var_key>.copy_info.src_tensor_desc` | object | 是 | 源 Tensor 描述；`CopyVarData` 使用其中的格式和类型构造源变量键，实际拷贝大小及类型转换取查找到的源资源条目 |
| `entries.<var_key>.init_data_offset` | integer | 是 | 初始化数据在权重文件中的字节偏移，无数据时为 0；`ParseVarEntryFromJson` 校验范围后按此偏移恢复 `entry.init_data`，供 `GetVarDevAddr` 首次初始化时拷贝到设备 |
| `entries.<var_key>.init_data_size` | integer | 是 | 初始化数据字节数，无数据时为 0；`ParseVarEntryFromJson` 用于切片和边界校验，恢复的数据由 `GetVarDevAddr` 经 `aclrtMemcpy` 初始化变量 |
| Tensor 描述的 `name` | string | 是 | Tensor 名称；`ParseTensorDescFromJson` 解析保存，当前变量键构造、格式转换和变量拷贝流程不使用此成员 |
| Tensor 描述的 `shape` | `array<integer>` | 是 | 各维度大小；资源条目中用于 `Init` 一致性校验和 `CopyVarData` 元素数计算，转换节点描述中用于 `TransVarOnHost`；`var_metas` 中仅保存 |
| Tensor 描述的 `data_type` / `format` | integer | 是 | 与 model_meta Tensor 描述采用相同枚举值；`RTVarBuildKey` 用于构造变量键，`Init` 用于一致性校验，`TransVarOnHost` / `CopyVarData` 用于确定转换参数；`var_metas` 中仅保存 |
| Tensor 描述的 `size` | integer | 是 | Tensor 字节数；`ParseTensorDescFromJson` 解析保存，当前运行时变量分配和拷贝主要使用资源条目的 `entries.<var_key>.size`，不读取此描述成员 |
| Tensor 描述的 `shape_range` | `array<array<integer>>` | 是 | 各维度的 `[min,max]`；`ParseTensorDescFromJson` 解析保存，当前变量地址准备、格式转换和变量拷贝流程不使用此成员，不是 `aclmdlGetInputDimsRange` 的数据来源 |

### 2.6 debug/ge_visual_00000000_graph_0.json

由 `VisualJsonConverter::SerializeFromGeModel` 生成，使用外层包装加 GE 模型可视化对象。`model` 内部按模型 Proto 描述递归转换，实际出现的字段由模型内容决定；不是 model_meta 的简化 Tensor 格式。

本文件通过 `DeserializeGertVisualJson`、`Om2PackageHelper::ExtractVisualJson` 整体提取；普通模型加载的 `DeserializeGertModelData` 不读取此文件。下表中的模型及属性字段由 `VisualJsonConverter::LoadFromVisualJson` 递归转换回 Proto JSON，供模型可视化和调试工具使用，不作为 `aclmdlGetDesc`、模型执行或 AIPP 查询的数据来源。

| JSON 字段 | 类型 | 必选 | 说明 |
|-----------|------|------|------|
| `format` | string | 是 | 固定为 `ge_visual_json`；供工具识别格式，当前 `LoadFromVisualJson` 不据此字段校验或选择转换逻辑 |
| `format_version` | integer | 是 | 当前为 1；记录可视化格式版本，当前 `LoadFromVisualJson` 不据此分支或执行版本校验 |
| `model` | object | 是 | GE 模型的可视化表示，字段源自 `ModelDef`；`LoadFromVisualJson` 检查该字段存在，并以 `ModelDef` 描述符启动递归转换 |
| `model.name` | string | 否（源模型提供时写入） | 模型名称；`LoadFromVisualJson` 经 `FromVisualRecursive` 恢复模型名称，供调试展示，无对应 ACL 名称查询 |
| `model.graph` | `array<object>` | 否（源模型包含图时写入） | 图集合；`FromVisualRecursive` 按 Proto 图字段描述递归恢复各图，供可视化工具展示 |
| `model.graph[].name` | string | 否（源图提供时写入） | 图名称；`FromVisualRecursive` 恢复图名称，供调试展示 |
| `model.graph[].op` | `array<object>` | 否（源图包含算子时写入） | 图中的算子集合；`FromVisualRecursive` 按算子描述递归恢复，供图结构展示 |
| `model.graph[].op[].name` / `type` | string | 否（源算子提供时写入） | 算子名称、类型；`FromVisualRecursive` 恢复算子标识，供图节点展示，不供 `aclmdlGetOpAttr` 查询 |
| `model.graph[].op[].input` | `array<string>` | 否（源算子包含输入连接时写入） | 算子输入连接信息；`FromVisualRecursive` 恢复连接描述，供可视化工具关联节点 |
| `model.graph[].op[].input_desc` / `output_desc` | `array<object>` | 否（源算子包含对应 Tensor 描述时写入） | 模型 Proto Tensor 描述经递归转换后的对象；`FromVisualRecursive` 恢复 Tensor 描述，供节点信息展示，不供 ACL 模型输入输出查询 |
| 各层 `attr` | object | 否（存在对应属性时写入） | 属性名到可视化属性值的映射；`ProcessObjectFromVisual` / `FromVisualRecursive` 按属性描述递归还原 Proto 属性 |
| 属性标量值 | string / integer / number / boolean | 否（存在标量属性时写入） | 标量属性使用对应 JSON 值；`WrapAttrDef` 恢复对应 Proto 属性包装 |
| 复合属性的 `type` | string | 是 | 非标量属性类型标识，如 `bytes`、`expression`、`tensor_desc`、`tensor`、`graph`、`func`；列表直接使用下述 `list_*` 标识；`WrapAttrDef` 据此识别属性类型 |
| 复合属性的 `value` | string / object / array | 是 | 与类型对应的内容，嵌套对象递归转换；`WrapAttrDef` 结合 `type` 还原属性内容，`FromVisualRecursive` 递归处理嵌套对象 |
| 列表值的 `type` | string | 是 | 如 `list_string`、`list_int`、`list_float`、`list_tensor_desc` 等；`WrapAttrDef` / `WrapListValue` 据此选择列表元素的 Proto 字段 |
| 列表值的 `value` | array | 是 | 对应类型的元素数组；`WrapListValueByType` 恢复列表包装，`ConvertListValueElementsFromVisual` 递归恢复消息类型元素 |
| 二维整数/浮点数组属性 | `array<array<number>>` | 否（存在对应二维数组属性时写入） | 直接输出嵌套数组，不包含 `type`/`value` 包装；`WrapAttrDef` 按首元素类型判断整数或浮点二维列表，再通过 `WrapNestedList` 恢复 Proto 包装 |
