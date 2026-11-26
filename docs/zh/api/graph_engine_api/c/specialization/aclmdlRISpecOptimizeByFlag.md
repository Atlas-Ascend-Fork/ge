# aclmdlRISpecOptimizeByFlag

## 产品支持情况

<!-- npu="950" id3 -->
- Ascend 950PR&950DT系列产品：支持
<!-- end id3 -->
<!-- npu="A3" id4 -->
- Atlas A3系列产品：支持
<!-- end id4 -->
<!-- npu="910b" id5 -->
- Atlas A2系列产品：支持
<!-- end id5 -->
<!-- npu="310b" id6 -->
- Atlas 200I/500 A2推理产品：不支持
<!-- end id6 -->
<!-- npu="310p" id7 -->
- Atlas推理系列产品：不支持
<!-- end id7 -->
<!-- npu="910" id9 -->
- Atlas训练系列产品：不支持
<!-- end id9 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/specialization/aclmdlRISpecOptimizeByFlag_res.md#id1 -->

## 头文件

\#include <aclmodel\_specialization.h\>

## 功能说明

对模型中由[aclmdlRISpecScopeBegin](aclmdlRISpecScopeBegin.md)和[aclmdlRISpecScopeEnd](aclmdlRISpecScopeEnd.md)标记范围内的Kernel进行常量化编译，并将编译结果更新到原模型中。Scope范围外的Kernel不参与本接口的常量化编译。

## 函数原型

```cpp
aclError aclmdlRISpecOptimizeByFlag(aclmdlRI modelRI, aclmdlRISpecOptions *options)
```

## 参数说明

**表1**  接口参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :---: | :--- |
| modelRI | 输入 | 待按范围常量化的模型句柄，不能为空。可通过[aclmdlRICaptureEnd](https://gitcode.com/cann/runtime/blob/9.3.0-beta.2/docs/zh/api_ref/15_model_running_instance_management.md#aclmdlRICaptureEnd)获取。本接口直接更新该模型，不创建新模型。 |
| options | 输入 | 常量化编译配置，传入nullptr时使用默认配置。配置结构体、成员及默认行为与[aclmdlRISpecOptimize参数说明](aclmdlRISpecOptimize.md#table-aclmdl-ri-spec-options)一致。配置中的Kernel名称规则只设置编译参数，不扩大Scope范围。 |

## 返回值说明

返回值类型为aclError，ACL_SUCCESS表示优化流程完成，其他值表示失败。没有可常量化的Kernel，或所有Kernel均被跳过时，也可返回ACL_SUCCESS。

| 返回值 | 描述 |
| --- | --- |
| ACL_ERROR_INVALID_PARAM | 配置参数无效、模型句柄为空或Scope标记配对无效。 |
| ACL_ERROR_BAD_ALLOC | 内存分配失败或容器长度超限。 |
| ACL_ERROR_FAILURE | 发生其他异常。 |

## 约束说明

- 模型须已完成捕获并处于空闲状态。调用期间，调用方须保证没有对该模型的并发执行、修改或销毁。
- 同一模型仅允许使用[aclmdlRISpecOptimize](aclmdlRISpecOptimize.md)与aclmdlRISpecOptimizeByFlag中的一个接口进行优化，且最多调用一次。
- options及其引用的数组、字符串在调用期间须保持有效且不变。数组的实际可读长度须与数量字段一致，字符串须以空字符结尾。
- Kernel须具有可用的常量化资源标识和参数信息，并能获取编译所需的参数值。不满足条件的Kernel保留原函数；单个Kernel的常量化编译失败时，记录告警并继续处理其他Kernel。
- 常量化编译成功的Kernel仅替换任务的函数句柄，保留原任务的其他下发参数。后续运行模型时，参与常量化的值须保持不变。
- 编译产物加载失败或找不到对应Kernel入口时，返回相应错误码，此时尚未修改模型。任务替换、模型更新或资源托管失败时，返回相应错误码，模型可能已被修改，且不进行回滚。调用失败时，可查看ERROR级别日志，定位失败阶段及相关模型或任务。
- 调用方取得模型句柄后，无论优化是否成功，均须在不再使用模型时调用[aclmdlRIDestroy](https://gitcode.com/cann/runtime/blob/9.3.0-beta.2/docs/zh/api_ref/15_model_running_instance_management.md#aclmdlRIDestroy)。优化成功后，常量化编译生成的Kernel资源随模型销毁自动释放。
- 起始标记与对应结束标记须在同一次模型捕获中成对使用。Scope范围依据模型任务序号确定，起止标记可位于不同Stream，范围内各Stream上的Kernel均参与常量化候选筛选。
- 支持多段范围和嵌套范围，结束标记与按模型任务序号排列的最近一个尚未闭合的起始标记配对。范围取并集，同一任务每次调用最多发起一次常量化编译。
- 模型中没有Scope标记，或Scope范围内没有Kernel时，不执行Kernel常量化编译。
- 识别到的标记存在多余结束标记或未闭合起始标记时，返回ACL_ERROR_INVALID_PARAM，不进行常量化编译或模型修改。
- 标记本身不参与常量化编译。常量化Scope须使用[aclmdlRISpecScopeBegin](aclmdlRISpecScopeBegin.md)与[aclmdlRISpecScopeEnd](aclmdlRISpecScopeEnd.md)成对标记。

## 调用示例

以下片段演示捕获两次`AddCustom`调用，并通过`aclmdlRISpecOptimizeByFlag`对标记范围内的第二次调用进行常量化编译。

调用前需完成[aclInit](https://gitcode.com/cann/runtime/blob/9.3.0-beta.2/docs/zh/api_ref/02_initialization_and_deinitialization.md#aclInit)、[aclrtSetDevice](https://gitcode.com/cann/runtime/blob/9.3.0-beta.2/docs/zh/api_ref/04_device_management.md#aclrtSetDevice)、[aclrtCreateStream](https://gitcode.com/cann/runtime/blob/9.3.0-beta.2/docs/zh/api_ref/06_stream_management.md#aclrtCreateStream)，创建x、y、t、z对应的Tensor并准备输入数据。

workspace1、workspace2和modelRI为输出参数。调用方负责失败时的捕获清理和资源释放，并保证Tensor关联的内存及workspace在模型销毁前保持有效。

示例函数返回0表示成功，返回1表示调用失败。

```cpp
#include "acl/acl.h"
#include "aclmodel_specialization.h"
#include "aclnn_add_custom.h"
#include <cstdio>

#define ASCENDC_CHECK(expr)                                                           \
    do {                                                                             \
        const auto checkRet = (expr);                                                 \
        if (checkRet != 0) {                                                          \
            std::fprintf(stderr, "%s failed: %d\n", #expr, static_cast<int>(checkRet)); \
            return 1;                                                                \
        }                                                                            \
    } while (false)

// x、y为输入，t为中间结果，z为输出。
// 两次AddCustom依次计算t = x + y、z = t + y。
int OptimizeByScopeWithAclnn(
    const aclTensor *x, const aclTensor *y,
    const aclTensor *t, const aclTensor *z,
    aclrtStream stream, void *&workspace1, void *&workspace2,
    aclmdlRI &modelRI)
{
    uint64_t workspaceSize1 = 0;
    uint64_t workspaceSize2 = 0;
    aclOpExecutor *executor1 = nullptr;
    aclOpExecutor *executor2 = nullptr;
    workspace1 = nullptr;
    workspace2 = nullptr;
    modelRI = nullptr;

    // 第一段：获取workspace大小和executor。
    ASCENDC_CHECK(aclnnAddCustomGetWorkspaceSize(x, y, t, &workspaceSize1, &executor1));
    ASCENDC_CHECK(aclnnAddCustomGetWorkspaceSize(t, y, z, &workspaceSize2, &executor2));

    if (workspaceSize1 > 0) {
        ASCENDC_CHECK(aclrtMalloc(&workspace1, workspaceSize1, ACL_MEM_MALLOC_NORMAL_ONLY));
    }
    if (workspaceSize2 > 0) {
        ASCENDC_CHECK(aclrtMalloc(&workspace2, workspaceSize2, ACL_MEM_MALLOC_NORMAL_ONLY));
    }

    // 在同一Stream上捕获两次AddCustom调用。
    ASCENDC_CHECK(aclmdlRICaptureBegin(stream, ACL_MODEL_RI_CAPTURE_MODE_THREAD_LOCAL));

    // 第一次AddCustom在Scope外。
    ASCENDC_CHECK(aclnnAddCustom(workspace1, workspaceSize1, executor1, stream));

    ASCENDC_CHECK(aclmdlRISpecScopeBegin(stream));

    // 第二次AddCustom在Scope内。
    ASCENDC_CHECK(aclnnAddCustom(workspace2, workspaceSize2, executor2, stream));

    ASCENDC_CHECK(aclmdlRISpecScopeEnd(stream));

    ASCENDC_CHECK(aclmdlRICaptureEnd(stream, &modelRI));

    // 对Scope范围内的Kernel进行常量化编译。
    ASCENDC_CHECK(aclmdlRISpecOptimizeByFlag(modelRI, nullptr));
    return 0;
}
```
