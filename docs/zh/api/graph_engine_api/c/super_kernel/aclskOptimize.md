# aclskOptimize

## 产品支持情况

<!-- npu="950" id1 -->
- Ascend 950PR&950DT系列产品：支持
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3系列产品：支持
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2系列产品：支持
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2推理产品：不支持
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas推理系列产品：不支持
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas训练系列产品：不支持
<!-- end id6 -->
<!-- npu="IPV350" id7 -->
- IPV350：不支持
<!-- end id7 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/super_kernel/aclskOptimize_res.md#id1 -->

## 头文件

\#include <super\_kernel/super\_kernel.h\>

## 功能说明

对模型运行实例进行SuperKernel融合优化。

本接口基于Kernel提供的SuperKernel适配二进制及相应元数据信息，识别模型运行实例中满足融合条件的Task，将其构建为SuperKernel Task，并使用优化后的Task更新原模型运行实例。

可配合[aclskScopeBegin](aclskScopeBegin.md)和[aclskScopeEnd](aclskScopeEnd.md)标记参与或不参与SuperKernel融合的范围；SuperKernel根据Task类型、算子适配情况、数据依赖和同步安全约束确定最终融合范围。

## 函数原型

```c
aclError aclskOptimize(aclmdlRI model, aclskOptions *options)
```

## 参数说明

| 参数名 | 输入/输出 | 描述 |
| --- | --- | --- |
| model | 输入 | 模型运行实例。类型定义请参见[aclmdlRI](https://gitcode.com/cann/runtime/blob/9.2.0/docs/zh/api_ref/25-05_Typedefs.md#aclmdlRI)。接口调用成功后，更新为优化后的模型运行实例。 |
| options | 输入 | SuperKernel优化选项。传入nullptr时使用默认配置。配置方法参见[选项说明](#选项说明)。 |

## 返回值说明

返回ACL_SUCCESS表示优化成功，返回其他值表示优化失败。

## 选项说明

### aclskOptions

`aclskOptions`表示传递给`aclskOptimize`的选项集合。

```c
struct aclskOption {
  aclskOptionType optionType;
  union {
    aclskDcciOption dcciBeforeKernelStart;
    aclskDcciOption dcciAfterKernelEnd;
    aclskDcciOption disableKernelDcci;
    aclskUbufLockIgnoreKernelOption ubufLockIgnoreKernel;
    aclskAutoOpParallelOption autoOpParallel;
    aclskEarlyStartOption earlyStart;
    aclskAggressiveOptStrategies aggressiveOpts;
    aclskDebugSyncAllOption debugSync;
    aclskDebugOpExecTraceOption debugOpExecTrace;
    aclskDebugCrossCoreSyncCheckOption debugCrossCoreSyncCheck;
    aclskDebugPerOpMaxCoreNumOption debugPerOpMaxCoreNum;
  };
};

typedef struct aclskOptions {
    aclskOption *options;
    size_t numOptions;
} aclskOptions;
```

| 成员 | 描述 |
| --- | --- |
| options | 指向包含 `numOptions` 个元素的 `aclskOption` 数组。 |
| numOptions | `options` 数组中的元素数量。 |

`options`及其引用的数组和字符串需保持有效，直至`aclskOptimize`返回。

`aclskOption`中各成员的配置说明参见[优化选项](#优化选项)和[调试选项](#调试选项)。

### 优化选项

以下结构体用于设置SuperKernel优化选项。

```c
typedef struct aclskDcciOption {
    char **kernelNames;
    size_t kernelCnt;
} aclskDcciOption;

typedef struct aclskUbufLockIgnoreKernelOption {
    uint32_t ubufLockIgnoreKernelCnt;
    char **ubufLockIgnoreKernel;
} aclskUbufLockIgnoreKernelOption;

typedef struct aclskAutoOpParallelOption {
    uint32_t enableAutoOpParallel;
} aclskAutoOpParallelOption;

typedef struct aclskEarlyStartOption {
    uint32_t enableEarlyStart;
} aclskEarlyStartOption;

typedef struct aclskAggressiveOptStrategies {
    uint32_t eventBreakerBypass;
    uint32_t valueBreakerBypass;
    uint32_t taskBreakerBypass;
} aclskAggressiveOptStrategies;
```

| `aclskOptionType` | 对应成员 | 参数说明 |
| --- | --- | --- |
| `DCCI_BEFORE_KERNEL_START` | `dcciBeforeKernelStart` | 默认不配置。`kernelNames`设置Kernel函数名匹配表达式数组，匹配规则参见[Kernel函数名匹配规则](#kernel函数名匹配规则)；`kernelCnt`设置数组元素数量。匹配的子Kernel在SuperKernel调用前执行全量Data Cache刷新。 |
| `DCCI_AFTER_KERNEL_END` | `dcciAfterKernelEnd` | 默认不配置。`kernelNames`设置Kernel函数名匹配表达式数组，匹配规则参见[Kernel函数名匹配规则](#kernel函数名匹配规则)；`kernelCnt`设置数组元素数量。匹配的子Kernel在SuperKernel调用后执行全量Data Cache刷新。 |
| `DCCI_DISABLE_ON_KERNEL` | `disableKernelDcci` | 默认不配置。`kernelNames`设置Kernel函数名匹配表达式数组，匹配规则参见[Kernel函数名匹配规则](#kernel函数名匹配规则)；`kernelCnt`设置数组元素数量。匹配的子Kernel已在内部配置Data Cache刷新，SuperKernel省略其调用后的默认全量Data Cache刷新。同一个Kernel同时匹配该选项和`DCCI_BEFORE_KERNEL_START`或`DCCI_AFTER_KERNEL_END`时，后两者优先生效。 |
| `UBUF_LOCK_IGNORE_KERNEL` | `ubufLockIgnoreKernel` | 默认不配置。`ubufLockIgnoreKernel`设置Kernel函数名匹配表达式数组，匹配规则参见[Kernel函数名匹配规则](#kernel函数名匹配规则)；`ubufLockIgnoreKernelCnt`设置数组元素数量。匹配的MIX Kernel不依赖UB Buffer的加锁、解锁机制保证数据访问顺序，SuperKernel据此优化融合策略。该选项在Atlas A2系列产品和Atlas A3系列产品上不生效。 |
| `AUTO_OP_PARALLEL` | `autoOpParallel.enableAutoOpParallel` | `0`（默认值）：关闭。<br>`1`：开启多流融合场景中的Cube/Vector算子自动并行排布，SuperKernel在保持算子依赖关系的前提下调整算子排布。 |
| `EARLY_START` | `earlyStart.enableEarlyStart` | `0`（默认值）：关闭。<br>`1`：开启，SuperKernel根据随子Kernel二进制携带的Early-Start能力信息进行调度优化。 |
| `AGGRESSIVE_OPT_STRATEGIES` | `aggressiveOpts` | `eventBreakerBypass`：预留字段。<br>`taskBreakerBypass`：<br>• `0`（默认值）：遇到非AICore Task时断开SuperKernel融合。<br>• `1`：融合范围内遇到非AICore Task，且对应流范围内没有其他AICore算子时，不断开融合范围，仅将该非AICore Task从融合范围中排除。<br>`valueBreakerBypass`为比特位组合，用于控制ValueWait节点的融合策略：<br>• `bit0`（值为`1`）：已配对且通过死锁检测的ValueWait节点参与融合。<br>• `bit1`（值为`2`）：未配对的ValueWait节点参与融合。<br>取值实例：<br>• `0`（默认值）：ValueWait节点不参与融合。<br>• `1`：仅允许已配对的ValueWait节点参与融合。<br>• `2`：仅允许未配对的ValueWait节点参与融合。<br>• `3`：同时允许已配对和未配对的ValueWait节点参与融合。<br>说明：一个ValueWait节点仅支持与一个ValueWrite节点配对。 |

### 调试选项

以下结构体用于设置SuperKernel调试选项。

```c
typedef struct aclskDebugSyncAllOption {
    uint32_t debugSyncAll;
} aclskDebugSyncAllOption;

typedef struct aclskDebugOpExecTraceOption {
    uint32_t enableOpExecTrace;
} aclskDebugOpExecTraceOption;

typedef struct aclskDebugCrossCoreSyncCheckOption {
    uint32_t enableCrossCoreSyncCheck;
} aclskDebugCrossCoreSyncCheckOption;

typedef struct aclskDebugPerOpMaxCoreNumOption {
    uint32_t enableDebugPerOpMaxCoreNum;
} aclskDebugPerOpMaxCoreNumOption;
```

| `aclskOptionType` | 对应成员 | 参数说明 |
| --- | --- | --- |
| `DEBUG_SYNC_ALL` | `debugSync.debugSyncAll` | `0`（默认值）：关闭。<br>`1`：在SuperKernel内的子算子之间插入`SyncAll`，用于定位同步问题。 |
| `DEBUG_OP_EXEC_TRACE` | `debugOpExecTrace.enableOpExecTrace` | `0`（默认值）：关闭。<br>`1`：记录SuperKernel入口及各子算子的执行进度，用于定位执行异常。 |
| `DEBUG_CROSS_CORE_SYNC_CHECK` | `debugCrossCoreSyncCheck.enableCrossCoreSyncCheck` | `0`（默认值）：关闭。<br>`1`：检查子算子的Cube/Vector核间同步数量是否匹配，同时启用`DEBUG_OP_EXEC_TRACE`。 |
| `DEBUG_PER_OP_MAX_CORE_NUM` | `debugPerOpMaxCoreNum.enableDebugPerOpMaxCoreNum` | `0`（默认值）：关闭。<br>`1`：将融合范围内的算子分别构建为独立SuperKernel，并按设备最大核数执行验证，同时启用`DEBUG_CROSS_CORE_SYNC_CHECK`。该选项会改变融合范围划分和启动核数，仅用于单算子功能验证。 |

### Kernel函数名匹配规则

Kernel函数名指编译生成的Kernel符号名，不是算子类型名。匹配表达式仅支持`.*`格式的正则表达式。使用Kernel函数名进行匹配的选项均遵循此匹配规则。

示例：

```c
char kernelNamePattern[] = ".*GroupedMatmul.*";
char *kernelNames[] = {kernelNamePattern};

aclskDcciOption option = {
    kernelNames,
    1
};
/* 可匹配的Kernel函数名：MyGroupedMatmulKernel */
```

## 约束说明

- 在模型运行实例构建完成后调用`aclskOptimize`。
- `aclskOptimize`调用成功后，模型运行实例仅用于执行或销毁，不得继续修改模型运行实例或再次调用`aclskOptimize`。

## 调用示例

以下示例展示在业务函数中捕获业务算子、标记SuperKernel融合范围、优化并执行模型运行实例的主要调用流程。Stream初始化及算子参数准备过程省略。

```c
#include <acl/acl.h>
#include <super_kernel/super_kernel.h>

aclmdlRI model;

aclmdlRICaptureBegin(stream, ACL_MODEL_RI_CAPTURE_MODE_GLOBAL);
aclskScopeBegin("scope", stream);
/* 调用需要参与SuperKernel融合的业务算子 */
aclskScopeEnd("scope", stream);
aclmdlRICaptureEnd(stream, &model);

aclError ret = aclskOptimize(model, NULL);
if (ret != ACL_SUCCESS) {
    /* 处理优化失败 */
    return;
}
aclmdlRIExecuteAsync(model, stream);
```
