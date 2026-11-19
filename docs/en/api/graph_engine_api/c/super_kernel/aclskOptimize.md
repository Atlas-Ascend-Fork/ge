# aclskOptimize

<!-- md-trans-meta sourceCommit=c0c24c5cd1c86bd859c92aa19eb866b3cff12abf translatedAt=2026-11-17T15:55:01.929Z pushedAt=2026-11-18T07:00:45.697Z nl=2 -->

## Product Support

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Not supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Not supported
<!-- end id6 -->
<!-- npu="IPV350" id7 -->
- IPV350: not supported
<!-- end id7 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/super_kernel/aclskOptimize_res.md#id1 -->

## Header File

\#include <super\_kernel/super\_kernel.h\>

## Function Description

Performs SuperKernel fusion optimization on a model running instance.

Based on the SuperKernel adaptation binary and corresponding metadata provided by the Kernel, this API identifies Tasks in the model running instance that meet the fusion conditions, builds them as SuperKernel Tasks, and updates the original model running instance with the optimized Tasks.

It can be used together with [aclskScopeBegin](aclskScopeBegin.md) and [aclskScopeEnd](aclskScopeEnd.md) to mark the scope that participates in or does not participate in SuperKernel fusion. SuperKernel determines the final fusion scope according to the Task type, operator adaptation status, data dependency, and synchronization safety constraints.

## Prototype

```c
aclError aclskOptimize(aclmdlRI model, aclskOptions *options)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Model running instance. See [aclmdlRI](https://gitcode.com/cann/runtime/blob/9.2.0/docs/en/api_ref/25-05_Typedefs.md#aclmdlRI) for the type definition. After a successful call, it is updated to the optimized model running instance. |
| options | Input | SuperKernel optimization option. When nullptr is passed, the default configuration is used. See [Option Description](#option-description) for the configuration method. |

## Return Value

Returns ACL_SUCCESS on success; otherwise, returns a failure value.

## Option Description

### aclskOptions

`aclskOptions` indicates the set of options passed to `aclskOptimize`.

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

| Member | Description |
| --- | --- |
| options | Points to an `aclskOption` array containing `numOptions` elements. |
| numOptions | Element count in the `options` array. |

`options` and the arrays and strings it references must remain valid until `aclskOptimize` returns.

See [Optimization Option](#optimization-option) and [Debug Options](#debug-options) for the configuration of each member in `aclskOption`.

### Optimization Option

The following structure is used to set the SuperKernel optimization option.

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

| `aclskOptionType` | Corresponding Member | Parameters |
| --- | --- | --- |
| `DCCI_BEFORE_KERNEL_START` | `dcciBeforeKernelStart` | Not configured by default. `kernelNames` sets the Kernel function name matching expression array. See [Kernel Function Name Matching Rule](#kernel-function-name-matching-rule) for the matching rule. `kernelCnt` sets the element count of the array. The matched sub-Kernels perform a full Data Cache flush before the SuperKernel call. |
| `DCCI_AFTER_KERNEL_END` | `dcciAfterKernelEnd` | Not configured by default. `kernelNames` sets the Kernel function name matching expression array. See [Kernel Function Name Matching Rule](#kernel-function-name-matching-rule) for the matching rule. `kernelCnt` sets the element count of the array. The matched sub-Kernels perform a full Data Cache flush after the SuperKernel call. |
| `DCCI_DISABLE_ON_KERNEL` | `disableKernelDcci` | Not configured by default. `kernelNames` sets the Kernel function name matching expression array. See [Kernel Function Name Matching Rule](#kernel-function-name-matching-rule) for the matching rule. `kernelCnt` sets the element count of the array. The matched sub-Kernels have already configured the Data Cache flush internally, so SuperKernel omits the default full Data Cache flush after their calls. When the same Kernel matches this option and `DCCI_BEFORE_KERNEL_START` or `DCCI_AFTER_KERNEL_END` simultaneously, the latter two take effect first. |
| `UBUF_LOCK_IGNORE_KERNEL` | `ubufLockIgnoreKernel` | Not configured by default. `ubufLockIgnoreKernel` sets the Kernel function name matching expression array. See [Kernel Function Name Matching Rule](#kernel-function-name-matching-rule) for the matching rule. `ubufLockIgnoreKernelCnt` sets the element count of the array. The matched MIX Kernels do not depend on the UB Buffer lock and unlock mechanism to guarantee data access order, and SuperKernel optimizes the fusion strategy accordingly. This option does not take effect on Atlas A2 products and Atlas A3 products. |
| `AUTO_OP_PARALLEL` | `autoOpParallel.enableAutoOpParallel` | `0` (default value): Disable.<br>`1`: Enable automatic parallel arrangement of Cube/Vector operators in multi-stream fusion scenarios. SuperKernel adjusts the operator arrangement while maintaining operator dependencies. |
| `EARLY_START` | `earlyStart.enableEarlyStart` | `0` (default value): Disable.<br>`1`: Enable. SuperKernel performs scheduling optimization according to the Early-Start capability information carried with the sub-Kernel binary. |
| `AGGRESSIVE_OPT_STRATEGIES` | `aggressiveOpts` | `eventBreakerBypass`: Reserved field.<br>`taskBreakerBypass`:<br>• `0` (default value): Break the SuperKernel fusion when a non-AICore Task is encountered.<br>• `1`: When a non-AICore Task is encountered within the fusion scope and there is no other AICore operator in the corresponding stream scope, do not break the fusion scope; only exclude the non-AICore Task from the fusion scope.<br>`valueBreakerBypass` is a bit combination used to control the fusion strategy of ValueWait nodes:<br>• `bit0` (value `1`): Paired ValueWait nodes that pass deadlock detection participate in fusion.<br>• `bit1` (value `2`): Unpaired ValueWait nodes participate in fusion.<br>Value examples:<br>• `0` (default value): ValueWait nodes do not participate in fusion.<br>• `1`: Only paired ValueWait nodes are allowed to participate in fusion.<br>• `2`: Only unpaired ValueWait nodes are allowed to participate in fusion.<br>• `3`: Both paired and unpaired ValueWait nodes are allowed to participate in fusion.<br>Description: A ValueWait node supports pairing with only one ValueWrite node. |

### Debug Options

The following structure is used to set the SuperKernel debugging option.

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

| `aclskOptionType` | Corresponding Member | Parameters |
| --- | --- | --- |
| `DEBUG_SYNC_ALL` | `debugSync.debugSyncAll` | `0` (default value): Disable.<br>`1`: Insert `SyncAll` between sub-operators within the SuperKernel to locate synchronization issues. |
| `DEBUG_OP_EXEC_TRACE` | `debugOpExecTrace.enableOpExecTrace` | `0` (default value): Disable.<br>`1`: Record the execution progress of the SuperKernel entry and each sub-operator to locate execution exceptions. |
| `DEBUG_CROSS_CORE_SYNC_CHECK` | `debugCrossCoreSyncCheck.enableCrossCoreSyncCheck` | `0` (default value): Disable.<br>`1`: Check whether the Cube/Vector inter-core synchronization counts of sub-operators match, and enable `DEBUG_OP_EXEC_TRACE` simultaneously. |
| `DEBUG_PER_OP_MAX_CORE_NUM` | `debugPerOpMaxCoreNum.enableDebugPerOpMaxCoreNum` | `0` (default value): Disable.<br>`1`: Build the operators within the fusion scope as independent SuperKernels respectively, execute and verify them with the maximum number of device cores, and enable `DEBUG_CROSS_CORE_SYNC_CHECK` simultaneously. This option changes the fusion scope division and the number of launched cores, and is used only for single-operator function verification. |

### Kernel Function Name Matching Rule

The Kernel function name refers to the Kernel symbol name generated by compilation, not the operator type name. The matching expression supports only regular expressions in the `.*` format. All options that use the Kernel function name for matching follow this matching rule.

Example:

```c
char kernelNamePattern[] = ".*GroupedMatmul.*";
char *kernelNames[] = {kernelNamePattern};

aclskDcciOption option = {
    kernelNames,
    1
};
/* Matchable Kernel function name: MyGroupedMatmulKernel */
```

## Constraints

- Call `aclskOptimize` after the model running instance is built.
- After a successful `aclskOptimize` call, the model running instance is used only for execution or destruction. Do not continue to modify the model running instance or call `aclskOptimize` again.

## Examples

The following example shows the main call flow of capturing business operators in a business function, marking the SuperKernel fusion scope, optimizing, and executing the model running instance. The Stream initialization and operator parameter preparation are omitted.

```c
#include <acl/acl.h>
#include <super_kernel/super_kernel.h>

aclmdlRI model;

aclmdlRICaptureBegin(stream, ACL_MODEL_RI_CAPTURE_MODE_GLOBAL);
aclskScopeBegin("scope", stream);
/* Call the business operators that need to participate in SuperKernel fusion */
aclskScopeEnd("scope", stream);
aclmdlRICaptureEnd(stream, &model);

aclError ret = aclskOptimize(model, NULL);
if (ret != ACL_SUCCESS) {
    /* Handle the optimization failure */
    return;
}
aclmdlRIExecuteAsync(model, stream);
```
