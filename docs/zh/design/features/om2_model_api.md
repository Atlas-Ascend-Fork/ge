# OM2 头文件参考

本文统一介绍 `om2_model_api.h`、`gert_model_data.h` 和 `om2_package_contants.h`。第一部分为模型 SO 的运行时 C API，第二部分为模型数据结构与包路径常量。

## 第一部分：OM2 模型 SO C API（om2_model_api.h）

### 1. 文档定位

本文档介绍 `inc/framework/om2/model_api/om2_model_api.h` 定义的全部跨边界数据结构与导出函数。该头文件是 **OM2 模型 SO 与执行器（Executor）之间的运行时 C ABI 契约**：模型 SO 实现并导出本文件声明的 C API，Executor 通过 `dlopen + dlsym` 按符号名绑定调用。

下文的 pbody 指 OM2 包内由代码生成器生成的模型动态库，即模型 SO。

- **兼容规则**：结构布局与符号冻结规则见 [OM2 兼容性规则](../../../../inc/framework/om2/README.md) 第 1~8 节，本文不重复规则内容，仅在必要处引用。
- **结构体尺寸**：`struct_size` 表示本次提供的结构体字节范围，通常初始化为 `sizeof(实际提供的结构体类型)`，不是独立的版本编号。
- **布局填充**：`abi_pad_N` 为固定布局填充，不承载业务数据，不得用于传递配置。

### 2. pbody 侧接口（模型 SO 导出，Executor 调用）

pbody 侧接口是模型 SO 实现并导出的 C 函数，Executor 在 `dlopen` 后通过 `dlsym` 按符号名绑定调用。

#### 2.1 GertModelHandle —— 模型实例不透明句柄

```c
typedef void *GertModelHandle;
```

- **功能说明**：模型实例的不透明句柄，由 `GertModelLoad` 的 `model_handle` 参数输出（Executor 提供存储、pbody 写入），有效期至 `GertModelUnload` 返回。
- Executor 不得解引用该句柄、不得假设其内部布局；句柄的创建/销毁责任全部在 pbody 侧。

#### 2.2 GertModelLoad

- **功能说明**：创建模型实例并完成资源初始化与 task 下沉。pbody 读取 `config` 中传入的算子二进制、常量/变量地址与外部资源，注册二进制、申请 Device 内存；成功后向 `*model_handle` 写入实例句柄。初始化期间 pbody 可通过 `config->callbacks` 调用 Executor 回调。
- **函数原型**：

```c
int32_t GertModelLoad(const GertModelLoadConfig *config, GertModelHandle *model_handle, GertModelLoadOutput *output);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| config | const GertModelLoadConfig * | 输入 | 加载参数，不允许为空，字段见下表 |
| model_handle | GertModelHandle * | 输出 | 不允许为空；调用前必须将 `*model_handle` 初始化为 `nullptr`。pbody 写入实例句柄，成功加载后用于执行，卸载后失效 |
| output | GertModelLoadOutput * | 预留 | 当前必须传 `nullptr`，pbody 会校验拒绝非空 |

- **返回值说明**：`int32_t`，`0` 表示成功；非 `0` 为失败码。失败后不得执行模型；当前实现可能在任务下沉失败前已写入实例句柄，此时应调用 `GertModelUnload` 清理非空句柄。

- **参数结构体 GertModelLoadConfig**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(GertModelLoadConfig)` |
| bin_files | const char ** | 算子二进制文件路径数组（UTF-8 字符串） |
| bin_data | const void ** | 算子二进制内存地址数组（内存方式加载二进制时填写） |
| bin_size | uint64_t * | 每个二进制的大小，单位字节 |
| bin_num | uint64_t | 二进制个数，即 bin_files/bin_data/bin_size 数组长度 |
| constants | void ** | 常量（权重）Device 地址数组，个数与模型常量数一致，无常量时可空 |
| var_addrs | void ** | 变量 Device 地址数组，无变量时可空 |
| work_ptr | void * | 工作空间地址（用户管理内存场景由 Executor 申请后传入），不使用时可空 |
| session_id | uint64_t * | Session ID 指针 |
| model_id | uint64_t | 模型 ID，仅用于日志与 DFX |
| instance_handle | void * | 执行器实例句柄，pbody 调用回调时原样回传作为第一个参数 |
| callbacks | const GertModelLoadCallbacks * | 加载期回调集合，不允许为空；二进制缓存路径使用的回调必须提供，具体可空性见第 3 章 |
| priority | int64_t | 模型优先级 |
| reuse_zero_copy | uint64_t | 是否复用零拷贝资源，`0` 否、非 `0` 是 |
| external_rt_model | aclmdlRI | 外部运行时模型句柄，外部资源管理场景传入 |
| external_streams | aclrtStream * | 外部流数组，长度为 external_stream_num |
| external_stream_num | uint64_t | 外部流个数 |
| external_events | aclrtEvent * | 外部 Event 数组，长度为 external_event_num |
| external_event_num | uint64_t | 外部 Event 个数 |
| external_labels | aclrtLabel * | 外部 Label 数组，长度为 external_label_num |
| external_label_num | uint64_t | 外部 Label 个数 |
| external_notifies | aclrtNotify * | 外部 Notify 数组，长度为 external_notify_num |
| external_notify_num | uint64_t | 外部 Notify 个数 |

- **参数结构体 GertModelLoadCallbacks**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(GertModelLoadCallbacks)` |
| report_model_base_info | ReportModelBaseInfoFunc | 上报模型运行时句柄，见 3.1 |
| launch_func | GertModelLaunchFunc | 下发单个任务，见 3.2 |
| lock_bin_handle_store | LockBinHandleStoreFunc | 锁定二进制句柄缓存，见 3.3 |
| unlock_bin_handle_store | UnlockBinHandleStoreFunc | 解锁二进制句柄缓存，见 3.3 |
| query_bin_handle_from_store | QueryBinHandleFromStoreFunc | 按 bin_id 查询缓存句柄，见 3.4 |
| save_bin_handle_to_store | SaveBinHandleToStoreFunc | 将句柄写入缓存，见 3.5 |
| release_bin_handle_from_store | ReleaseBinHandleFromStoreFunc | 释放缓存句柄，见 3.6 |
| hccl_load_task | HcclLoadTaskFunc | 下发 HCCL 通信任务（HCOM/HVD），见 3.8 |

- **参数结构体 GertModelLoadOutput**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 结构体字节数；当前无其他字段，本结构体为预留，Load 的 output 参数必须传 `nullptr` |

#### 2.3 GertModelRun

- **功能说明**：同步执行已加载的模型：更新输入输出相关地址和参数表后，通过 `aclmdlRIExecute` 执行已构建模型，当前线程阻塞直至执行完成，超时由 `config->stream_sync_timeout_ms` 控制。逐任务下发发生在加载阶段，不在每次执行时重复进行。
- **函数原型**：

```c
int32_t GertModelRun(GertModelHandle model_handle, const GertModelRunConfig *config, GertModelRunOutput *output);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| model_handle | GertModelHandle | 输入 | `GertModelLoad` 返回的有效句柄 |
| config | const GertModelRunConfig * | 输入 | 执行参数，不允许为空，字段见下表 |
| output | GertModelRunOutput * | 预留 | 当前实现不读取或回填该参数，可传 `nullptr`；Executor 当前传入已初始化的预留输出结构体 |

- **返回值说明**：`int32_t`，`0` 表示成功；非 `0` 为失败码。

- **参数结构体 GertModelRunConfig**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(GertModelRunConfig)` |
| input_count | uint64_t | 模型输入个数 |
| input_data | gert::Tensor ** | 模型输入 Tensor 指针数组，长度为 input_count |
| output_count | uint64_t | 模型输出个数 |
| output_data | gert::Tensor ** | 模型输出 Tensor 指针数组（由 Executor 填写输出地址），长度为 output_count |
| stream_sync_timeout_ms | uint64_t | 同步执行的流同步超时，单位毫秒 |
| run_callbacks | const GertModelRunCallbacks * | 执行期 DFX 回调集合，见下表；具体回调语义见 3.7 |

- **参数结构体 GertModelRunCallbacks**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(GertModelRunCallbacks)` |
| report_run_info_preprocess | ReportModelRunFunc | 模型执行前 DFX 回调，见 3.7 |
| report_run_info_postprocess | ReportModelRunFunc | 模型执行后 DFX 回调，见 3.7 |

- **参数结构体 GertModelRunOutput**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 结构体字节数；传入预留输出对象时初始化为 `sizeof(GertModelRunOutput)`，当前不回填结果 |

#### 2.4 GertModelRunAsync

- **功能说明**：异步执行已加载的模型：更新输入输出相关地址和参数表后，通过 `aclmdlRIExecuteAsync` 将模型执行提交到 `stream` 指定的流，不等待 Device 执行完成。
- **函数原型**：

```c
int32_t GertModelRunAsync(GertModelHandle model_handle, aclrtStream stream, const GertModelRunConfig *config,
                          GertModelRunOutput *output);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| model_handle | GertModelHandle | 输入 | `GertModelLoad` 返回的有效句柄 |
| stream | aclrtStream | 输入 | 异步执行使用的流 |
| config | const GertModelRunConfig * | 输入 | 执行参数，结构体同 2.3 的 GertModelRunConfig |
| output | GertModelRunOutput * | 预留 | 当前实现不读取或回填该参数，可传 `nullptr`；Executor 当前传入已初始化的预留输出结构体 |

- **返回值说明**：`int32_t`，`0` 表示成功（仅代表下发完成）；非 `0` 为失败码。

#### 2.5 GertModelUnload

- **功能说明**：销毁模型实例并释放其管理的资源（Device 内存、算子二进制引用、流等）。成功卸载后 `model_handle` 失效；Executor 在 dlclose 模型 SO 前必须先完成 Unload。
- **函数原型**：

```c
int32_t GertModelUnload(GertModelHandle model_handle, const GertModelUnloadConfig *config,
                        GertModelUnloadOutput *output);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| model_handle | GertModelHandle | 输入 | 待清理的模型实例句柄，成功卸载后失效；传值调用不会将调用方保存的句柄自动置空 |
| config | const GertModelUnloadConfig * | 预留 | 当前必须传 `nullptr`，非空时返回失败且不销毁实例 |
| output | GertModelUnloadOutput * | 预留 | 当前必须传 `nullptr` |

- **返回值说明**：`int32_t`，`0` 表示成功；非 `0` 为失败码。

- **参数结构体 GertModelUnloadConfig / GertModelUnloadOutput**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 结构体字节数；当前无其他字段，Unload 的 config 和 output 参数均必须传 `nullptr` |

#### 2.6 GertModelRefreshFeatureMap

- **功能说明**：根据新的 FeatureMap 基地址，更新 Host 参数表中可刷新的 FeatureMap 地址，后续执行时再将参数表拷贝到 Device。模型加载成功后调用；本接口不刷新权重地址。
- **函数原型**：

```c
int32_t GertModelRefreshFeatureMap(GertModelHandle model_handle, uintptr_t base_addr);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| model_handle | GertModelHandle | 输入 | `GertModelLoad` 返回的有效句柄 |
| base_addr | uintptr_t | 输入 | 新的 FeatureMap 基地址，不得为 0 |

- **返回值说明**：`int32_t`，`0` 表示成功；非 `0` 为失败码。

#### 2.7 GertModelGetStreamNum

- **功能说明**：返回模型所需的 stream 数量。该数量在编译期固化，不接收 model_handle、不依赖模型加载状态，dlsym 之后、SO 存活期内任意时刻调用均返回一致结果。外部资源管理场景可据此创建流，再通过 `GertModelLoadConfig::external_streams` 传入模型。
- **函数原型**：

```c
uint64_t GertModelGetStreamNum();
```

- **参数说明**：无。
- **返回值说明**：`uint64_t`，模型所需的 stream 数量。

#### 2.8 GertModelGetStreamDesc

- **功能说明**：向输出数组填充编译期固化的 stream flags，用于创建流。当 `extended_attrs` 为空时，不依赖模型实例；传入 `GertModelStreamDesc` 时，还会回填已加载模型的内部流，需要有效的模型实例句柄。
- **函数原型**：

```c
int32_t GertModelGetStreamDesc(uint32_t *stream_flags, uint64_t stream_num, void *extended_attrs);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| stream_flags | uint32_t * | 输出 | 不允许为空，pbody 填充每个 stream 的 flags |
| stream_num | uint64_t | 输入 | 数组长度，必须与 `GertModelGetStreamNum` 返回值一致，不一致时报错 |
| extended_attrs | void * | 输入/输出 | 可传 `nullptr`；也可传 `GertModelStreamDesc *`，调用前填写有效的 `model_handle`，返回内部流数组与数量 |

- **返回值说明**：`int32_t`，`0` 表示成功；非 `0` 为失败码。

- **参数结构体 GertModelStreamDesc**：通过 `extended_attrs` 传入，用于查询已加载模型的内部流。Executor 当前使用该能力设置模型流优先级。

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| model_handle | GertModelHandle | `GertModelLoad` 成功返回、Unload 前的有效句柄 |
| streams | aclrtStream * | 回填，指向模型内部流数组，只读，Load 后至 Unload 前有效 |
| stream_num | uint64_t | 回填实际流个数；旧版本 pbody 不回填，保持调用前值 |

#### 2.9 GertModelGetEventNum

- **功能说明**：返回模型所需的 event 数量，语义同 2.7（编译期固化、无状态查询）。
- **函数原型**：

```c
uint64_t GertModelGetEventNum();
```

- **参数说明**：无。
- **返回值说明**：`uint64_t`，模型所需的 event 数量。

#### 2.10 GertModelGetEventDesc

- **功能说明**：向输出数组填充编译期确定的 event flags，不依赖已加载的模型实例。当前不使用 `extended_attrs`，没有流描述查询中的实例扩展功能。
- **函数原型**：

```c
int32_t GertModelGetEventDesc(uint32_t *event_flags, uint64_t event_num, void *extended_attrs);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| event_flags | uint32_t * | 输出 | Executor 提供的输出数组，pbody 填充每个 event 的 flags |
| event_num | uint64_t | 输入 | 数组长度，必须与 `GertModelGetEventNum` 返回值一致，不一致时报错 |
| extended_attrs | void * | 预留 | 当前传 `nullptr` |

- **返回值说明**：`int32_t`，`0` 表示成功；非 `0` 为失败码。

#### 2.11 GertModelGetLabelNum

- **功能说明**：返回模型所需的 label 数量，语义同 2.7（编译期固化、无状态查询）。当前无对应的 `GetLabelDesc` 接口。
- **函数原型**：

```c
uint64_t GertModelGetLabelNum();
```

- **参数说明**：无。
- **返回值说明**：`uint64_t`，模型所需的 label 数量。

#### 2.12 GertModelGetNotifyNum

- **功能说明**：返回模型所需的 notify 数量，语义同 2.7（编译期固化、无状态查询）。
- **函数原型**：

```c
uint64_t GertModelGetNotifyNum();
```

- **参数说明**：无。
- **返回值说明**：`uint64_t`，模型所需的 notify 数量。

#### 2.13 GertModelGetNotifyDesc

- **功能说明**：向输出数组填充编译期确定的 notify flags，不依赖已加载的模型实例。当前不使用 `extended_attrs`，没有流描述查询中的实例扩展功能。
- **函数原型**：

```c
int32_t GertModelGetNotifyDesc(uint64_t *notify_flags, uint64_t notify_num, void *extended_attrs);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| notify_flags | uint64_t * | 输出 | Executor 提供的输出数组，pbody 填充每个 notify 的 flags |
| notify_num | uint64_t | 输入 | 数组长度，必须与 `GertModelGetNotifyNum` 返回值一致，不一致时报错 |
| extended_attrs | void * | 预留 | 当前传 `nullptr` |

- **返回值说明**：`int32_t`，`0` 表示成功；非 `0` 为失败码。

### 3. Executor 侧回调接口（Executor 实现，pbody 调用）

回调函数由 Executor 实现并通过 `GertModelLoadConfig::callbacks`（加载期）与 `GertModelRunConfig::run_callbacks`（执行期）注入 pbody。回调统一返回 `int32_t`，返回值如何处理见各回调说明。除 bin handle 缓存组回调（3.3~3.6）外，回调第一个参数均为 `void *instance_handle`，其值为 `GertModelLoadConfig::instance_handle` 原样回传，Executor 用它定位自身实例。回调在当前调用线程内同步执行；RunAsync 表示 Device 执行异步，不表示回调在后台线程执行。

可空性按用途区分：`GertModelLoadConfig::callbacks` 本身不得为空；二进制缓存路径使用的 lock/unlock/query/save/release 回调必须提供，当前生成代码会直接调用。`report_model_base_info` 可为空，未提供则跳过上报；`launch_func` 可为空，未提供时由生成代码直接下发。`run_callbacks` 及其中的执行前后回调可为空，未提供则跳过。回调入参由模型 SO 准备，需要由 Executor 回填的字段在说明中注明。

#### 3.1 report_model_base_info（ReportModelBaseInfoFunc）

- **功能说明**：pbody 在 `GertModelLoad` 过程中完成运行时资源初始化、获得 `aclmdlRI` 句柄后调用本回调上报；Executor 用该句柄关联 dump、profiling 等运行时信息。
- **函数原型**：

```c
int32_t (*ReportModelBaseInfoFunc)(void *instance_handle, const GertModelBaseInfo *info);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| instance_handle | void * | 输入 | `GertModelLoadConfig::instance_handle` 原样回传 |
| info | const GertModelBaseInfo * | 输入 | 模型基础信息，pbody 填写，字段见下表 |

- **参数结构体 GertModelBaseInfo**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(GertModelBaseInfo)` |
| rt_model_handle | void * | codegen 侧创建的 `aclmdlRI *`，pbody 完成资源初始化（InitResources）后即可获得 |

- **返回值说明**：`0` 表示成功；非 `0` 表示上报失败，pbody 据此中止加载流程。

#### 3.2 launch_func（GertModelLaunchFunc）

- **功能说明**：模型加载期，pbody 在 `GertModelLoad` 的任务构建/下沉过程中，将生成的任务经本回调交由 Executor 下发。本回调属于 Executor 侧的 DFX 回调集合，在使能 DFX 功能时由 Executor 组装，经 `GertModelLoadCallbacks::launch_func` 传入 pbody；回调为空时，生成代码直接调用 ACL/RT 接口下发任务。Executor 按 `launch_info->launch_type` 选择对应下发途径，完成后可回填 `launch_info->task_info->task_id`，供 dump/profiling 关联。回调在加载调用线程内执行，后续 Run/RunAsync 通过模型执行接口运行已构建的任务。
- **函数原型**：

```c
int32_t (*GertModelLaunchFunc)(void *instance_handle, GertModelTaskLaunchInfo *launch_info);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| instance_handle | void * | 输入 | `GertModelLoadConfig::instance_handle` 原样回传 |
| launch_info | GertModelTaskLaunchInfo * | 输入/输出 | 任务下发信息，pbody 填写（task_id 可由 Executor 回填），字段见下表 |

- **参数结构体 GertModelTaskLaunchInfo**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(GertModelTaskLaunchInfo)` |
| launch_type | GertModelTaskLaunchType | 下发类型，决定 launch_params 指向的联合体成员，取值见下表 |
| task_info | GertModelTaskDesc * | 任务 DFX 信息，字段见下表；其中 task_id 可由 Executor 下发后回填 |
| launch_params | const GertModelTaskLaunchParams * | 与下发类型对应的参数（联合体），见下表 |

- **参数结构体涉及的枚举 GertModelTaskLaunchType（下发类型）**：

| 枚举值 | 数值 | 下发途径 |
|--------|------|---------|
| ACL_RT_LAUNCH_KERNEL_V2 | 0 | 通过 `aclrtLaunchKernelV2` 下发 |
| RT_STARS_TASK_LAUNCH_WITH_FLAG | 1 | 通过 `rtStarsTaskLaunchWithFlag` 下发 |
| ACL_RT_LAUNCH_CUSTOM_KERNEL | 2 | 通过自定义算子的回调函数下发 |
| ACL_RT_LAUNCH_HCCL | 3 | 通过 Executor 侧 HCCL 适配层下发（静态离线通信任务） |

- **参数结构体 GertModelTaskLaunchParams**：本身无 `struct_size`，由 `launch_type` 决定读取哪个成员；各成员结构体独立携带 `struct_size`。

| 字段 | 类型 | 说明 |
|------|------|------|
| launch_kernel_v2_params | GertModelLaunchKernelV2Params | `launch_type` 为 `ACL_RT_LAUNCH_KERNEL_V2` 时使用 |
| launch_stars_task_params | GertModelLaunchStarsTaskWithFlagParams | `launch_type` 为 `RT_STARS_TASK_LAUNCH_WITH_FLAG` 时使用 |
| launch_custom_kernel_params | GertModelLaunchCustomKernelParams | `launch_type` 为 `ACL_RT_LAUNCH_CUSTOM_KERNEL` 时使用 |
| launch_hccl_params | GertModelLaunchHcclParams | `launch_type` 为 `ACL_RT_LAUNCH_HCCL` 时使用 |

**GertModelLaunchKernelV2Params**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| func_handle | aclrtFuncHandle | 待下发的 kernel 函数 |
| block_dim | uint32_t | kernel block dim |
| abi_pad_1 | uint32_t | ABI 填充，保持与 ACL 接口布局一致 |
| args_data | const void * | kernel 参数地址 |
| args_size | size_t | kernel 参数大小，单位字节 |
| config | aclrtLaunchKernelCfg * | kernel 下发配置 |
| stream | aclrtStream | 下发流 |

**GertModelLaunchStarsTaskWithFlagParams**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| task_sqe | const void * | Task SQE 地址 |
| sqe_len | uint32_t | SQE 长度，单位字节 |
| abi_pad_1 | uint32_t | ABI 填充，保持与 ACL 接口布局一致 |
| stream | aclrtStream | 下发流 |
| flag | uint32_t | 下发标志 |
| abi_pad_2 | uint32_t | ABI 填充，保持与 ACL 接口布局一致 |

**GertModelLaunchCustomKernelParams**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| func_launch_custom_kernel | uint32_t (*)(void *, void *) | 自定义算子下发回调函数 |
| eager_op | void * | 自定义算子对象 |
| eager_op_context | void * | 自定义算子执行入参 |

**GertModelLaunchHcclParams**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| func_load_hccl | HcclLoadTaskFunc | HCCL 任务下发函数，取 Executor 注入的宿主 `hccl_load_task` 回调，见 3.8 |
| task_params | const HcclTaskParams * | 扁平化 HCCL 任务参数，字段见 3.8 |

- **参数结构体 GertModelTaskDesc**：单个算子任务的 DFX 信息，Executor 侧用于 L0 异常 dump 与 profiling 上报。

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| op_name | const char * | 算子名称，不允许为空 |
| op_type | const char * | 算子类型，不允许为空 |
| task_id | uint64_t | 运行时 task ID，Executor 下发完成后回填，供 dump/profiling 关联 |
| stream_id | uint64_t | 运行时 stream ID |
| context_id | uint64_t | 运行时 context ID |
| thread_id | uint64_t | 运行时 thread ID |
| block_dim | uint64_t | kernel launch block dim，非 kernel 任务填 0 |
| op_desc_id | uint64_t | OpDesc ID |
| args_base | uintptr_t | args buffer 基地址 |
| args_size | uint64_t | args buffer 大小，单位字节 |
| input_num | uint64_t | inputs 指向的输入 Tensor 个数 |
| inputs | const GertModelTaskIoEntry * | 输入 Tensor 映射数组首地址，input_num 为 0 时可空 |
| output_num | uint64_t | outputs 指向的输出 Tensor 个数 |
| outputs | const GertModelTaskIoEntry * | 输出 Tensor 映射数组首地址，output_num 为 0 时可空 |
| workspace_num | uint64_t | Workspace 个数 |
| workspace_addrs | const uint64_t * | Workspace 地址数组首地址，个数为 0 时可空 |
| workspace_sizes | const uint64_t * | Workspace 大小数组首地址，单位字节，个数为 0 时可空 |
| task_type | uint64_t | 任务类型，取值与 `ModelTaskType` 一致 |
| kernel_type | uint64_t | kernel 类型，取值与 `ge::ccKernelType` 一致，默认 10000（INVALID） |
| stream | void * | rtStream_t 运行时流句柄 |
| is_raw_address | uint64_t | 是否 raw address 模式，`0` 否、非 `0` 是 |
| task_raw_info | const GertModelTaskRawInfo * | L0 异常 dump 原始信息，不需要时可空 |
| launch_begin | uint64_t | kernel launch 开始时间戳 |
| original_op_names | const char * | 分号分隔的原始算子名称列表，非融合算子为 nullptr |
| input_mem_size | uint64_t | 输入内存大小，单位字节 |
| output_mem_size | uint64_t | 输出内存大小，单位字节 |
| workspace_mem_size | uint64_t | workspace 内存大小，单位字节 |
| weight_mem_size | uint64_t | 权重内存大小，单位字节 |
| op_impl_mode | uint64_t | 算子实现模式，取值与 GE `_op_impl_mode_enum` 属性一致，`0` 为默认模式 |
| prof_ge_task_type | uint64_t | profiling 上报任务类型，取值与 `MsprofGeTaskType`（aprof_pub.h）一致，`0` 表示 AI_CORE |
| prof_block_dim | uint64_t | profiling 口径的 block dim（未归一/mix 编码/tiling_sink 占位），与 block_dim（launch 口径）分离 |

- **参数结构体 GertModelTaskIoEntry**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| tensor | gert::Tensor * | Tensor 基础信息指针，不允许为空 |
| offset | uint64_t | Tensor 地址在 args buffer 中的偏移，单位字节 |

- **参数结构体 GertModelTaskRawInfo**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| need_assert_or_printf | uint64_t | 是否需要 assert/printf 相关异常 dump，`0` 否、非 `0` 是 |
| arg_num | uint64_t | args 指向的槽位个数 |
| args | const GertModelArgSlotInfo * | 槽位信息数组首地址，arg_num 为 0 时可空 |

- **参数结构体涉及的枚举 GertModelArgKind（kernel 参数槽位类型）**：

| 枚举值 | 数值 | 含义 |
|--------|------|------|
| GERT_MODEL_ARG_INPUT | 0 | 输入 Tensor 地址槽位 |
| GERT_MODEL_ARG_OUTPUT | 1 | 输出 Tensor 地址槽位 |
| GERT_MODEL_ARG_WORKSPACE | 2 | Workspace 地址槽位 |
| GERT_MODEL_ARG_TILING | 3 | Tiling 数据地址槽位 |
| GERT_MODEL_ARG_SHAPE_INFO | 4 | Shape 信息地址槽位 |
| GERT_MODEL_ARG_LEVEL1_DESC | 5 | 一级描述信息地址槽位 |
| GERT_MODEL_ARG_PLACEHOLDER | 6 | 占位地址槽位 |
| GERT_MODEL_ARG_CUSTOM_VALUE | 7 | 自定义立即数槽位 |
| GERT_MODEL_ARG_FFTS_ADDR | 8 | FFTS 地址槽位 |
| GERT_MODEL_ARG_EVENT_ADDR | 9 | Event 地址槽位 |
| GERT_MODEL_ARG_OVERFLOW_ADDR | 10 | Overflow 地址槽位 |
| GERT_MODEL_ARG_EMPTY_ADDR | 11 | 空地址槽位 |
| GERT_MODEL_ARG_INVALID_KIND | 0xFFFF | 非法值 |

- **参数结构体 GertModelArgSlotInfo**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，通常取对应结构体的 `sizeof` |
| kind | GertModelArgKind | 槽位类型，取值见上表，默认 INVALID_KIND |
| flags | uint64_t | 预留标志位，当前填 0 |
| args_offset | uint64_t | 槽位在 args buffer 中的偏移，单位字节 |
| value | uint64_t | 槽位附加值：Tiling/Shape 场景为数据大小，其他场景按 kind 解释 |
| related_index | uint64_t | 关联索引：输入/输出/Workspace 场景为对应数组下标 |
| event_id | uint64_t | Event 场景的事件 ID，非 Event 场景填 0 |
| level1_target_offset | uint64_t | 一级描述信息指向的目标槽位偏移，单位字节 |

- **返回值说明**：`0` 表示成功；非 `0` 表示下发失败，pbody 中止本次加载流程。

#### 3.3 lock_bin_handle_store / unlock_bin_handle_store（LockBinHandleStoreFunc / UnlockBinHandleStoreFunc）

- **功能说明**：pbody 访问 Executor 侧的跨模型二进制句柄缓存时，先经 lock 回调进入临界区，操作完成后经 unlock 回调退出。加载时顺序为：`lock → query → 未命中时加载二进制 → save（记录本次引用）→ 获取函数句柄 → unlock`。命中缓存也必须调用 save 增加引用计数；卸载时在锁内调用 release（见 3.6）。
- **函数原型**：

```c
int32_t (*LockBinHandleStoreFunc)();
int32_t (*UnlockBinHandleStoreFunc)();
```

- **参数说明**：无。
- **返回值说明**：`0` 表示成功；非 `0` 为失败码。

#### 3.4 query_bin_handle_from_store（QueryBinHandleFromStoreFunc）

- **功能说明**：按 `bin_id` 查询 Executor 侧缓存中已加载的二进制句柄，命中时实现跨模型复用、避免重复加载。
- **函数原型**：

```c
int32_t (*QueryBinHandleFromStoreFunc)(const char *bin_id, aclrtBinHandle *bin_handle);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| bin_id | const char * | 输入 | 二进制标识字符串（UTF-8、`\0` 结束），pbody 提供，有效期覆盖调用 |
| bin_handle | aclrtBinHandle * | 输出 | pbody 提供存储；Executor 命中时写入句柄，未命中时写入 `nullptr` |

- **返回值说明**：`0` 表示查询成功，包含未命中的情况；是否命中通过 `*bin_handle` 是否为空判断。非 `0` 表示查询发生错误，生成代码据此中止加载。

#### 3.5 save_bin_handle_to_store（SaveBinHandleToStoreFunc）

- **功能说明**：记录模型对二进制句柄的引用。首次加载时将句柄写入缓存并将引用计数置为 1；命中已有缓存时增加引用计数。无论句柄来自缓存还是新加载，都需要调用本回调。
- **函数原型**：

```c
int32_t (*SaveBinHandleToStoreFunc)(const char *bin_id, aclrtBinHandle bin_handle);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| bin_id | const char * | 输入 | 二进制标识字符串（UTF-8、`\0` 结束），pbody 提供，有效期覆盖调用 |
| bin_handle | aclrtBinHandle | 输入 | 新加载或从缓存取得的二进制句柄 |

- **返回值说明**：`0` 表示成功；非 `0` 为失败码。

#### 3.6 release_bin_handle_from_store（ReleaseBinHandleFromStoreFunc）

- **功能说明**：pbody 卸载模型或不再使用某二进制时调用，Executor 减少引用计数并告知调用方是否需要真正卸载该二进制。
- **函数原型**：

```c
int32_t (*ReleaseBinHandleFromStoreFunc)(const char *bin_id, uint8_t *need_unload);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| bin_id | const char * | 输入 | 二进制标识字符串（UTF-8、`\0` 结束），pbody 提供，有效期覆盖调用 |
| need_unload | uint8_t * | 输出 | pbody 提供存储，Executor 回填；`1` 表示引用计数归零，pbody 应卸载二进制，`0` 表示无需卸载 |

- **返回值说明**：`0` 表示成功；非 `0` 为失败码。

#### 3.7 report_run_info_preprocess / report_run_info_postprocess（ReportModelRunFunc）

- **功能说明**：用于执行期 DFX（profiling 计时、dump 分发等）。提供回调且执行到对应位置时，preprocess 在参数表拷贝及模型执行提交前触发，postprocess 在模型执行接口成功返回后触发；前序步骤失败可能跳过后置回调。RunAsync 的后置回调表示执行提交返回，不代表 Device 已完成推理。
- **函数原型**：

```c
int32_t (*ReportModelRunFunc)(void *instance_handle, const GertModelRunReportInfo *info);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| instance_handle | void * | 输入 | `GertModelLoadConfig::instance_handle` 原样回传 |
| info | const GertModelRunReportInfo * | 输入 | 执行上报信息，pbody 填写，字段见下表 |

- **参数结构体 GertModelRunReportInfo**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(GertModelRunReportInfo)` |
| model_id | uint64_t | 模型 ID |
| stream | aclrtStream | 执行流；生成代码在同步 Run 时填 `nullptr`，仅异步执行（RunAsync）时填执行流 |
| is_async | uint64_t | 是否异步执行，`0` 同步、非 `0` 异步 |

- **返回值说明**：`0` 表示成功；非 `0` 表示回调失败。当前生成代码忽略这两个回调的返回值，不因返回值中止执行，也不在调用处据此记录错误；如需记录失败，由回调实现自行处理。

#### 3.8 hccl_load_task（HcclLoadTaskFunc）

- **功能说明**：静态离线模型的 HCCL 通信任务（HCOM/HVD/HVDWAIT）下发通道。模型加载期，pbody 在 `GertModelLoad` 的任务下沉过程中，将编译期固化的通信参数组装为 `HcclTaskParams`，经本回调交由 Executor 侧 HCCL 适配层执行 `PrepareTaskAsync` + `LoadTask` 并登记任务句柄；模型卸载时由 Executor 在调用 `GertModelUnload` 前统一反注册（`UnloadTask` 并释放句柄），不经模型 SO 回调。运行期下发场景下，本函数指针经 `GertModelLaunchHcclParams::func_load_hccl` 传递，见 3.2 联合体成员。
- **版本要求**：含 HCCL 通信算子的模型要求 `callbacks->struct_size` 覆盖 `hccl_load_task` 字段（即不小于 `offsetof(GertModelLoadCallbacks, hccl_load_task) + sizeof(HcclLoadTaskFunc)`），否则 `GertModelLoad` 直接失败并提示升级 Executor；本回调对含 HCCL 算子的模型为必选，为空时同样失败。
- **函数原型**：

```c
int32_t (*HcclLoadTaskFunc)(void *instance_handle, const HcclTaskParams *params);
```

- **参数说明**：

| 参数 | 类型 | 输入/输出 | 说明 |
|------|------|----------|------|
| instance_handle | void * | 输入 | `GertModelLoadConfig::instance_handle` 原样回传，回调实现据此定位执行器实例 |
| params | const HcclTaskParams * | 输入 | 扁平化通信任务参数，pbody 填写，字段见下表 |

- **参数结构体 HcclTaskParams**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(HcclTaskParams)` |
| hccl_type | const char * | 通信算子类型字符串（KernelHcclDef.hccl_type），不允许为空 |
| private_def | const void * | HCCL 私有定义（编译期静态字节） |
| private_def_len | uint32_t | 私有定义长度，单位为字节 |
| abi_pad_1 | uint32_t | ABI 填充 |
| main_stream | void * | 主流句柄 |
| input_addrs | void *const * | 输入地址数组（录制期解析） |
| input_num | uint32_t | 输入地址数量 |
| abi_pad_2 | uint32_t | ABI 填充 |
| output_addrs | void *const * | 输出地址数组（录制期解析） |
| output_num | uint32_t | 输出地址数量 |
| abi_pad_3 | uint32_t | ABI 填充 |
| workspace_addr | void * | workspace 地址，可为空 |
| workspace_size | uint64_t | workspace 字节数 |
| hccl_data_type | int32_t | HCCL 数据类型枚举数值（编译期转换） |
| hccl_reduce_op | int32_t | HCCL 归约类型枚举数值（编译期转换） |
| count | uint64_t | 通信元素数 |
| root_id | int32_t | Broadcast/Gather/Reduce root rank |
| abi_pad_4 | uint32_t | ABI 填充 |
| follow_streams | void *const * | follow 从流句柄数组（InitResources 按主流预创建） |
| follow_stream_num | uint32_t | follow 从流数量 |
| abi_pad_5 | uint32_t | ABI 填充 |
| attached_streams | void *const * | attached 从流句柄数组，可为空 |
| attached_stream_num | uint32_t | attached 从流数量 |
| need_refresh | uint8_t | 是否地址刷新模式（args 槽位间接寻址），`0` 否、非 `0` 是 |
| abi_pad_6 | uint8_t[3] | ABI 填充 |
| args_base | void * | args 基址（need_refresh 间接寻址模式），可为空 |
| abi_pad_7 | uint32_t | ABI 填充 |
| abi_pad_8 | uint32_t | ABI 填充（尾部显式对齐） |
| hvd_metas | const HcclHorovodMeta * | 非空 = HVD 多 info 模式（每输入一条 meta）；HCOM 为空 |
| hvd_wait | uint8_t | HVDWAIT 标记：仅 1 条 info（只记 hccl_type，无 IO），`0` 否、非 `0` 是 |
| is_collective | uint8_t | LoadTask 成功后缓存通信任务扩展信息（aclrtCacheLastTaskExtendInfo） |
| abi_pad_9 | uint8_t[6] | ABI 填充 |
| input_zero_copy_flags | const int32_t * | 每输入零拷贝标志（槽位化；长度 = input_num），可为空 |
| output_zero_copy_flags | const int32_t * | 每输出零拷贝标志（长度 = output_num），可为空 |
| overflow_addr | void * | 溢出检测地址（op 含 GLOBALWORKSPACE_TYPE 时非空，L3） |

- **参数结构体 HcclHorovodMeta（HVD 单输入元数据）**：

| 字段 | 类型 | 说明 |
|------|------|------|
| struct_size | uint64_t | 本次提供的结构体字节范围，调用前初始化为 `sizeof(HcclHorovodMeta)` |
| input_name | const char * | TF tensor 名（op_desc->GetInputNameByIndex），回调对接用 |
| dims | const int64_t * | 该输入维度数组（SO 内静态字节） |
| dims_num | uint32_t | 维度个数 |
| abi_pad_1 | uint32_t | ABI 填充 |
| hccl_data_type | int32_t | 该输入 HCCL 数据类型枚举数值（编译期转换，每输入独立） |
| abi_pad_2 | uint32_t | ABI 填充 |
| count | uint64_t | 该输入通信元素数（编译期按 GetHorovodCount 规则解析） |

- **返回值说明**：`0` 表示成功；非 `0` 表示 HCCL 任务下发失败，pbody 侧 `GertModelLoad` 中止并返回错误。

## 第二部分：OM2 模型数据

`gert_model_data.h` 和 `om2_package_contants.h` 共同约定 OM2 包的数据组织与目录结构：前者以 `GertModelData` 为根，描述包清单、各模型及共享资源；后者定义这些数据的包内路径、JSON 键名、文件类别和压缩规则。序列化模块按照这些约定，将模型数据写入 ZIP 归档。

理解包格式时，可先看 `GertModelData` 各成员对应哪些目录和文件，再查看成员引用的具体数据结构。这里的结构体描述落盘数据的组织关系，并非将 C++ 对象内存直接写入文件；JSON 字段和包内文件内容见 [OM2 模型包内部结构](internal_structure_of_om2_model_for_dev.md)。

### 1. gert_model_data.h

文件位置为 `inc/framework/om2/model_data/gert_model_data.h`。

以下从 `GertModelData` 开始，按成员声明顺序深度优先展开：先完整介绍 `manifest` 引用的结构及其子结构，再介绍 `models`，之后依次介绍 `constants`、`kernels`、`custom_ops`。每个数据结构体单独列出“字段、类型、说明”三列表；重复引用的类型只在首次出现时展开，后续引用对应小节。

智能指针和容器继续按其指向或包含的本头文件自定义类型展开，直到成员为基本类型、字符串或由基本类型组成的容器。外部头文件定义的 `ge::*` 类型注明来源，不在此重复展开。`struct_size` 默认初始化为对应结构体的 `sizeof`，`reserved[4]` 默认全零，两者均不写入 JSON。表格使用头文件中的完整 C++ 类型，不再使用 `String`、`Ptr<T>`、`Array<T>` 等缩写。

#### 1.1 GertModelData：OM2 包的数据根结构

`GertModelData` 汇总一个 OM2 包的全部数据，是理解落盘格式的入口。其成员与包内文件、目录对应如下，路径均相对归档根前缀 `<base_name>/`：

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数，默认初始化为 `sizeof(GertModelData)`，不写入归档 |
| `manifest` | `std::unique_ptr<GertModelDataManifest>` | 包清单，对应 `manifest.json` |
| `models` | `std::vector<std::unique_ptr<GertModelDataModel>>` | 模型集合，按下标对应 `data/model_0/`、`data/model_1/` 等目录；每个模型包含元信息、常量/变量配置、运行时代码和调试数据 |
| `constants` | `std::unique_ptr<GertModelDataConstants>` | 常量权重数据集合，对应 `data/constants/`；具体常量与模型的关联由模型的常量配置描述 |
| `kernels` | `std::unique_ptr<GertModelDataKernels>` | 算子二进制集合，对应 `data/kernels/` |
| `custom_ops` | `std::unique_ptr<GertModelDataCustomOps>` | 自定义算子产物集合，对应 `data/custom_ops/binaries_npu_arch/` 和 `data/custom_ops/shared_libs/` |

其中，`models` 的元素类型 `GertModelDataModel` 进一步描述单模型目录下的 `model_meta.json`、`constants_config.json`、`variables_config.json`、`op_attr.json`、`runtime/` 和 `debug/`。变量初始化数据位于包级 `data/variables/`，由模型变量配置中的文件名、偏移和大小关联，不是 `GertModelData` 的独立顶层成员。文件是否实际生成由对应数据和序列化条件决定。

#### 1.2 GertModelDataManifest

由 `GertModelData::manifest` 引用，对应 `manifest.json`。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `compatibility` | `GertModelDataCompatibility` | 编译侧、执行器及特性版本信息，见第 1.3 节 |
| `model_num` | `uint64_t` | 模型数量，默认 0；单模型为 1，Bundle 为子模型数 |
| `atc_command` | `std::unique_ptr<char[]>` | ATC 命令字符串 |

#### 1.3 GertModelDataCompatibility

由 `GertModelDataManifest::compatibility` 引用，对应包清单中的 `compatibility` 对象。

| 字段 | 类型 | 说明 |
|------|------|------|
| `compiler_version` | `std::unique_ptr<char[]>` | 编译侧版本 |
| `required_executor_version` | `std::unique_ptr<char[]>` | 模型所需执行器版本 |
| `used_features` | `std::map<std::unique_ptr<char[]>, std::unique_ptr<char[]>, UniquePtrCharCompare>` | 特性名到版本串的映射；键和值均为字符串，比较器见第 1.4 节 |
| `reserved` | `uint64_t[4]` | 保留 |

#### 1.4 UniquePtrCharCompare

`GertModelDataCompatibility::used_features` 使用的字符串比较器，无数据字段，因此不列字段表。它通过 `operator()` 调用 `strcmp` 比较两个 `std::unique_ptr<char[]>` 指向的字符串内容，不比较指针地址。

至此，`GertModelData::manifest` 引用的数据已展开完毕，下面继续介绍 `models`。

#### 1.5 GertModelDataModel

由 `GertModelData::models` 的元素引用，对应单个 `data/model_N/` 目录。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `model_meta` | `std::unique_ptr<GertModelDataModelMeta>` | 模型元信息，对应 `model_meta.json`，见第 1.6 节 |
| `constants_config` | `std::unique_ptr<GertModelDataConstantsConfig>` | 常量配置，对应 `constants_config.json`，见第 1.9 节 |
| `op_attr_json` | `std::unique_ptr<char[]>` | 算子属性 JSON 原文，对应 `op_attr.json` |
| `runtime` | `std::unique_ptr<GertModelDataRuntime>` | 运行时代码与模型 SO，对应 `runtime/`，见第 1.11 节 |
| `debug` | `std::unique_ptr<GertModelDataDebug>` | 调试数据，对应 `debug/`，见第 1.13 节 |
| `variables_config` | `std::unique_ptr<GertModelDataVariablesConfig>` | 变量配置，对应 `variables_config.json`，见第 1.14 节 |

#### 1.6 GertModelDataModelMeta

由 `GertModelDataModel::model_meta` 引用，保存模型元信息。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `work_size` | `uint64_t` | 工作内存字节数，默认 0 |
| `zero_copy_size` | `int64_t` | 输入输出内存优化相关的工作内存字节数，默认 0 |
| `dynamic_type` | `int64_t` | 动态分档类型，默认 0，对应 `dynamic_dims.dynamic_type` |
| `model_name` | `std::unique_ptr<char[]>` | 模型名称，对应 JSON 的 `name` |
| `input_desc` | `std::vector<GertTensorDesc>` | 输入描述，元素结构见第 1.7 节；动态分档时其 shape 写为 `max_gear_shape` |
| `output_desc` | `std::vector<GertTensorDesc>` | 输出描述，元素结构同第 1.7 节，对应 `outputs[]` |
| `input_desc_v2` | `std::vector<GertTensorDesc>` | AIPP 场景的输入描述，元素结构同第 1.7 节，对应 `shape_aclmdlGetInputDimsV2` |
| `output_desc_v2` | `std::vector<GertTensorDesc>` | 元素结构同第 1.7 节；无独立 JSON 字段，反序列化时由 `outputs[]` 派生 |
| `dynamic_batch_info` | `std::vector<std::vector<int64_t>>` | 档位输入信息，对应 `dynamic_dims.gears[].inputs` |
| `dynamic_output_shape` | `std::vector<std::unique_ptr<char[]>>` | 逗号分隔的档位/输出信息串，按档位分组转换为 `gears[].outputs` |
| `user_designate_shape_order` | `std::vector<std::unique_ptr<char[]>>` | 动态分档的用户指定 shape 顺序 |
| `origin_input_dims` | `std::vector<std::vector<int64_t>>` | 动态分档原始输入 shape，写入 `inputs[].shape` |
| `aipp_infos` | `std::vector<std::unique_ptr<GertModelDataAippMeta>>` | AIPP 元信息，对应 `aipp.aipp_infos[]`，元素结构见第 1.8 节 |

#### 1.7 GertTensorDesc

由 `GertModelDataModelMeta` 的输入输出描述数组引用，也供后续变量结构复用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `size` | `uint64_t` | Tensor 字节数，默认 0 |
| `data_type` | `ge::DataType` | 外部数据类型枚举，默认 `DT_UNDEFINED`，JSON 中为整数 |
| `format` | `ge::Format` | 外部格式枚举，默认 `FORMAT_ND`，JSON 中为整数 |
| `name` | `std::unique_ptr<char[]>` | Tensor 名称 |
| `shape` | `std::vector<int64_t>` | 各维度大小 |
| `shape_range` | `std::vector<std::pair<int64_t, int64_t>>` | 各维度的最小值、最大值 |
| `reserved` | `uint64_t[4]` | 保留 |

`ge::DataType` 和 `ge::Format` 的枚举定义见 `inc/framework/executor_c/types.h`，不属于本头文件自定义结构体。

#### 1.8 GertModelDataAippMeta

由 `GertModelDataModelMeta::aipp_infos` 的元素引用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `aipp_type` | `ge::InputAippType` | AIPP 类型，默认 `DATA_WITHOUT_AIPP` |
| `aipp_data_index` | `uint64_t` | 关联输入下标，默认 `kOm2InvalidAippDataIndex` |
| `aipp_config_info` | `std::unique_ptr<ge::AippConfigInfo>` | 序列化时展开为 AIPP 配置字段 |
| `aipp_input_dims` | `std::vector<std::unique_ptr<ge::InputOutputDims>>` | AIPP 输入描述，对应 `aipp_inputs` |
| `aipp_output_dims` | `std::vector<std::unique_ptr<ge::InputOutputDims>>` | AIPP 输出描述，对应 `aipp_outputs` |
| `orig_input_info` | `std::unique_ptr<ge::OriginInputInfo>` | 原始输入信息，展开为 `orig_input_format`、`orig_input_data_type`、`orig_input_dim_num` |

上述 `ge::*` 类型定义于 `inc/graph_metadef/common/ge_common/ge_types.h`，不在本节递归展开。至此，`model_meta` 引用的数据已展开完毕，下面继续介绍 `constants_config`。

#### 1.9 GertModelDataConstantsConfig

由 `GertModelDataModel::constants_config` 引用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `internal_weight_size` | `uint64_t` | 内置权重总字节数，默认 0 |
| `consts` | `std::vector<std::unique_ptr<GertModelDataConstMeta>>` | 常量元信息，元素结构见第 1.10 节；JSON 中为按常量键组织的对象 |

#### 1.10 GertModelDataConstMeta

由 `GertModelDataConstantsConfig::consts` 的元素引用；成员均为数值或字符串。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `index` | `uint64_t` | 常量下标，默认 0 |
| `offset` | `int64_t` | 数据源内字节偏移，默认 0 |
| `size` | `int64_t` | 常量字节数，默认 0 |
| `type` | `std::unique_ptr<char[]>` | 常量来源类型，如 `INTERNAL` |
| `file_name` | `std::unique_ptr<char[]>` | 数据文件名 |
| `file_path` | `std::unique_ptr<char[]>` | 外部文件路径，按序列化条件写入 |
| `op_name` | `std::unique_ptr<char[]>` | 算子名，非 INTERNAL 常量使用它作为 `consts` 的对象键 |

#### 1.11 GertModelDataRuntime

由 `GertModelDataModel::runtime` 引用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `so_artifact` | `GertModelDataFile` | 模型动态库，文件内容结构见第 1.12 节 |
| `source_artifacts` | `std::vector<GertModelDataFile>` | `runtime/csrc/` 下源码与构建文件，元素结构同第 1.12 节 |

#### 1.12 GertModelDataFile

由 `GertModelDataRuntime` 引用，也供包内常量、算子二进制及自定义算子数据复用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `file_name` | `std::unique_ptr<char[]>` | 文件基名，如 `constant_0`、`libxxx.so`、`xxx.cpp` |
| `data` | `ge::ReadonlyByteBuffer` | 文件内容，拥有/视图二态；视图不拥有底层内存，使用期间必须保持底层内存有效 |
| `data_size` | `uint64_t` | 内容字节数，默认 0 |
| `reserved` | `uint64_t[4]` | 保留 |

`ge::ReadonlyByteBuffer` 定义于 `inc/graph_metadef/common/ge_common/ge_types.h`，不在本节递归展开。

#### 1.13 GertModelDataDebug

由 `GertModelDataModel::debug` 引用；只包含结构体尺寸及 JSON 字符串。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `visual_json` | `std::unique_ptr<char[]>` | 可视化 JSON 原文 |

#### 1.14 GertModelDataVariablesConfig

由 `GertModelDataModel::variables_config` 引用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `graph_id` | `uint64_t` | 图 ID，默认 0 |
| `entries` | `std::vector<RTVarEntry>` | 变量资源，元素结构见第 1.15 节；JSON 中为对象 |
| `var_metas` | `std::vector<std::unique_ptr<GertModelDataVarMeta>>` | 变量元信息，元素结构见第 1.18 节 |
| `global_shared_var_size` | `uint64_t` | 全局共享变量 HBM 总字节数，默认 0；Bundle 工作区按子模型最大值分配 |

#### 1.15 RTVarEntry

由 `GertModelDataVariablesConfig::entries` 的元素引用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `var_name` | `std::unique_ptr<char[]>` | 变量名 |
| `var_key` | `std::unique_ptr<char[]>` | 变量键，同时作为 JSON `entries` 对象键 |
| `op_type` | `std::unique_ptr<char[]>` | 算子类型 |
| `logic_addr` | `uint64_t` | 逻辑地址，默认 0 |
| `size` | `uint64_t` | 变量字节数，默认 0 |
| `memory_type` | `uint64_t` | 内存类型，默认 0 |
| `tensor_desc` | `GertTensorDesc` | 变量描述，复用第 1.7 节 |
| `trans_road` | `RTVarTransRoad` | `std::vector<RTTransNodeInfo>` 的别名；转换路径，元素结构见第 1.16 节 |
| `changed_graph_id` | `uint64_t` | 变更关联图 ID，默认 0 |
| `allocated_graph_id` | `uint64_t` | 分配关联图 ID，默认 0 |
| `copy_info` | `RTCopyNodeInfo` | 变量拷贝信息，见第 1.17 节 |
| `extern_dev_addr` | `void *` | 外部 Device 地址，默认空，不序列化 |
| `init_data` | `std::vector<uint8_t>` | 初始化数据，写入变量权重文件，以 JSON 的偏移和大小定位 |
| `file_name` | `std::unique_ptr<char[]>` | 变量权重文件基名，反序列化时按名寻址 |
| `reserved` | `uint64_t[4]` | 保留 |

#### 1.16 RTTransNodeInfo

由 `RTVarEntry::trans_road` 的元素引用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `node_type` | `std::unique_ptr<char[]>` | 转换节点类型 |
| `input` | `GertTensorDesc` | 转换前描述，复用第 1.7 节 |
| `output` | `GertTensorDesc` | 转换后描述，复用第 1.7 节 |
| `reserved` | `uint64_t[4]` | 保留 |

#### 1.17 RTCopyNodeInfo

由 `RTVarEntry::copy_info` 引用。

| 字段 | 类型 | 说明 |
|------|------|------|
| `src_var_name` | `std::unique_ptr<char[]>` | 拷贝源变量名 |
| `src_tensor_desc` | `GertTensorDesc` | 拷贝源描述，复用第 1.7 节 |
| `reserved` | `uint64_t[4]` | 保留 |

#### 1.18 GertModelDataVarMeta

`entries` 引用的数据展开完毕后，继续介绍 `GertModelDataVariablesConfig::var_metas` 的元素类型。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `index` | `uint64_t` | 变量下标，默认 0 |
| `var_name` | `std::unique_ptr<char[]>` | 变量名 |
| `op_type` | `std::unique_ptr<char[]>` | 算子类型 |
| `tensor_desc` | `GertTensorDesc` | 变量描述，复用第 1.7 节 |
| `op_name` | `std::unique_ptr<char[]>` | 算子名 |

至此，`GertModelData::models` 引用的数据已展开完毕，下面依次介绍根结构的其余成员。

#### 1.19 GertModelDataConstants

由 `GertModelData::constants` 引用，对应包级 `data/constants/`。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `constants_data` | `std::vector<std::unique_ptr<GertModelDataFile>>` | 按模型下标关联 `constant_N`，外置权重场景元素可空；文件结构复用第 1.12 节 |

#### 1.20 GertModelDataKernels

由 `GertModelData::kernels` 引用，对应包级 `data/kernels/`。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `binaries` | `std::vector<std::unique_ptr<GertModelDataFile>>` | 算子二进制文件集合，文件结构复用第 1.12 节 |

#### 1.21 GertModelDataCustomOps

由 `GertModelData::custom_ops` 引用，对应包级 `data/custom_ops/`。

| 字段 | 类型 | 说明 |
|------|------|------|
| `struct_size` | `uint64_t` | 结构体字节数 |
| `binaries` | `std::vector<std::unique_ptr<GertModelDataFile>>` | `binaries_npu_arch/` 下的自定义算子二进制，文件结构复用第 1.12 节 |
| `libraries` | `std::vector<std::unique_ptr<GertModelDataFile>>` | `shared_libs/` 下的自定义算子动态库，文件结构复用第 1.12 节 |

#### 1.22 GertBuffer

根结构的递归介绍到此结束。`GertBuffer` 是本头文件定义的公共缓冲区，不是 `GertModelData` 的成员，单独补充如下。

| 字段 | 类型 | 说明 |
|------|------|------|
| `length` | `uint64_t` | 缓冲区字节数，默认 0 |
| `data` | `std::shared_ptr<uint8_t>` | 共享持有缓冲区，默认空 |
| `reserved` | `uint64_t[4]` | 保留 |

#### 1.23 公共常量

| 定义 | 类型/值 | 说明 |
|------|---------|------|
| `GERT_MODEL_DATA_API` | 导出可见性宏 | Windows 使用 `__declspec(dllexport)`，其他环境使用 default visibility |
| `GERT_EXECUTOR_VERSION` | `const char *`，`"1.0"` | manifest 版本校验基准 |
| `kOm2InvalidAippDataIndex` | `uint64_t`，`0xFFFFFFFF` | 无关联 AIPP 输入下标 |

### 2. om2_package_contants.h

文件位置为 `inc/framework/om2/model_data/om2_package_contants.h`。该文件将模型数据的组织关系落实为统一的包内路径和 JSON 键名，并通过文件类别与压缩白名单规定各类文件是否压缩。

#### 2.1 路径与 JSON 键常量

以下为全部 23 个字符串常量，类型均为 `inline constexpr const char *`，由 `FORALL_OM2_CONSTANTS` 宏统一定义。路径相对归档根前缀 `<base_name>/`；模型路径中的 `%s` 替换为模型编号，其他占位符见说明。

| 常量 | 值/路径模板 | 说明 |
|------|-------------|------|
| `OM2_MODEL_NUM` | `model_num` | 模型数量键 |
| `OM2_ATC_COMMAND` | `atc_command` | ATC 命令键 |
| `OM2_MANIFEST_PATH` | `manifest.json` | 包清单路径 |
| `OM2_MANIFEST_KEY_COMPATIBILITY` | `compatibility` | 版本信息对象键 |
| `OM2_MANIFEST_KEY_COMPILER_VERSION` | `compiler_version` | 编译器版本键 |
| `OM2_MANIFEST_KEY_REQUIRED_EXECUTOR_VERSION` | `required_executor_version` | 所需执行器版本键 |
| `OM2_MANIFEST_KEY_USED_FEATURES` | `used_features` | 使用特性映射键 |
| `OM2_DATA_DIR` | `data/` | 数据目录 |
| `OM2_MODEL_DIR_FORMAT` | `data/model_%s/` | 单模型目录 |
| `OM2_MODEL_META_PATH_FORMAT` | `data/model_%s/model_meta.json` | 模型元信息 |
| `OM2_RUNTIME_DIR_FORMAT` | `data/model_%s/runtime/` | 模型运行时目录 |
| `OM2_RUNTIME_CSRC_DIR_FORMAT` | `data/model_%s/runtime/csrc/` | 模型源码目录 |
| `OM2_DEBUG_DIR_FORMAT` | `data/model_%s/debug/` | 调试目录 |
| `OM2_OP_ATTR_PATH_FORMAT` | `data/model_%s/op_attr.json` | 算子属性 |
| `OM2_CUSTOM_KERNELS_DIR_FORMAT` | `data/custom_ops/%s/` | `%s` 为 `binaries_npu_arch` 或 `shared_libs` |
| `OM2_KERNELS_DIR` | `data/kernels/` | 算子二进制目录 |
| `OM2_CONSTANTS_DIR` | `data/constants/` | 常量目录 |
| `OM2_CONSTANTS_FILE_PREFIX` | `constant_` | 常量文件名前缀 |
| `OM2_CONSTANTS_CONFIG_PATH_FORMAT` | `data/model_%s/constants_config.json` | 常量配置 |
| `OM2_VARIABLES_CONFIG_PATH_FORMAT` | `data/model_%s/variables_config.json` | 变量配置 |
| `OM2_VARIABLES_DIR` | `data/variables/` | 变量权重目录 |
| `OM2_VAR_WEIGHT_FILE_FORMAT` | `data/variables/var_weight_data_%s` | 变量权重文件，`%s` 为模型索引 |
| `OM2_VISUAL_JSON_PATH_FORMAT` | `data/model_%s/debug/ge_visual_00000000_graph_0.json` | 可视化 JSON |

#### 2.2 GertFileType 与压缩白名单

`GertFileType` 定义包内文件类别，枚举值按声明顺序从 0 递增；`kGertCompressedFileWhitelist` 是需要压缩的文件类别白名单。打包时通过 `IsGertFileCompressed(type)` 判断：在白名单中使用 DEFLATE 压缩，不在白名单中使用 Stored 存储，不压缩。白名单仅决定压缩方式，不决定文件是否生成或是否允许打包。

| 文件类别 | 数值 | 包内文件 | 是否压缩 |
|----------|------|----------|----------|
| `kCsrc` | 0 | `data/model_%s/runtime/csrc/*` | 是，白名单内，DEFLATE |
| `kSo` | 1 | `data/model_%s/runtime/lib*.so` | 是，白名单内，DEFLATE |
| `kConstants` | 2 | `data/constants/constant_N` | 否，Stored |
| `kConstantsConfig` | 3 | `data/model_%s/constants_config.json` | 是，白名单内，DEFLATE |
| `kVarWeight` | 4 | `data/variables/var_weight_data_N` | 否，Stored |
| `kVariablesConfig` | 5 | `data/model_%s/variables_config.json` | 否，Stored |
| `kKernels` | 6 | `data/kernels/*` | 否，Stored |
| `kCustomKernels` | 7 | `data/custom_ops/*` | 否，Stored |
| `kModelMeta` | 8 | `data/model_%s/model_meta.json` | 否，Stored |
| `kOpAttr` | 9 | `data/model_%s/op_attr.json` | 是，白名单内，DEFLATE |
| `kVisualJson` | 10 | `data/model_%s/debug/ge_visual_*.json` | 是，白名单内，DEFLATE |
| `kManifest` | 11 | `manifest.json` | 否，Stored |

#### 2.3 工具函数与维护

| 定义 | 返回值 | 说明 |
|------|--------|------|
| `IsGertFileCompressed(GertFileType type)` | `bool` | `constexpr` 函数，类型在白名单内返回 true，否则 false |
| `FormatOm2Path(const char *fmt, Args... args)` | `std::string` | 按格式模板生成路径；计算长度或格式化失败返回空串 |
