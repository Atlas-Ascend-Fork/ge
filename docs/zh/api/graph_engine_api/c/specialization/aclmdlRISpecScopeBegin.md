# aclmdlRISpecScopeBegin

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
<!-- npu="IPV350" id10 -->
- IPV350：不支持
<!-- end id10 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/specialization/aclmdlRISpecScopeBegin_res.md#id1 -->

## 头文件

\#include <aclmodel\_specialization.h\>

## 功能说明

在指定Stream中下发常量化Scope起始标记，与[aclmdlRISpecScopeEnd](aclmdlRISpecScopeEnd.md)配合，限定[aclmdlRISpecOptimizeByFlag](aclmdlRISpecOptimizeByFlag.md)处理的任务范围。

## 函数原型

```cpp
aclError aclmdlRISpecScopeBegin(aclrtStream stream)
```

## 参数说明

**表1**  接口参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :---: | :--- |
| stream | 输入 | 下发起始标记的Stream句柄。不能为空。 |

## 返回值说明

返回值类型为aclError。ACL_SUCCESS表示已完成前置检查并尝试下发标记，不表示设备执行完成；下发过程中的同步错误通过日志记录。

| 返回值 | 描述 |
| --- | --- |
| ACL_ERROR_INVALID_PARAM | stream为空，或参数无效。 |
| ACL_ERROR_FEATURE_UNSUPPORTED | 当前架构不支持下发Scope标记。 |
| ACL_ERROR_BAD_ALLOC | 内存分配失败或容器长度超限。 |
| ACL_ERROR_FAILURE | 发生其他异常。 |

查询当前设备或设备架构失败时，返回对应的底层错误码。

## 约束说明

- 须在模型捕获期间调用，且传入参与该次捕获的Stream。本接口不检查Stream的捕获状态。
- 起始标记与对应结束标记须在同一次模型捕获中成对使用。Scope范围依据模型任务序号确定，起止标记可位于不同Stream，范围内各Stream上的Kernel均参与常量化候选筛选。
- 支持多段范围和嵌套范围。结束标记与按模型任务序号排列的最近一个尚未闭合的起始标记配对，关闭内层范围后，外层范围仍然有效。
- 起止标记的配对由[aclmdlRISpecOptimizeByFlag](aclmdlRISpecOptimizeByFlag.md)检查。存在多余结束标记或未闭合起始标记时，该优化接口返回ACL_ERROR_INVALID_PARAM。
- 起止标记之间没有计算任务时，不产生常量化候选。标记本身不参与常量化编译，范围内的Kernel仍须满足常量化条件。
- Scope范围仅对[aclmdlRISpecOptimizeByFlag](aclmdlRISpecOptimizeByFlag.md)生效，[aclmdlRISpecOptimize](aclmdlRISpecOptimize.md)不按Scope限制常量化范围。

## 调用示例

以下片段演示在第二次`AddCustom`调用前使用`aclmdlRISpecScopeBegin`标记常量化编译范围的起点。

调用前需完成[aclInit](https://gitcode.com/cann/runtime/blob/master/docs/zh/api_ref/02_initialization_and_deinitialization.md#aclInit)、[aclrtSetDevice](https://gitcode.com/cann/runtime/blob/master/docs/zh/api_ref/04_device_management.md#aclrtSetDevice)、[aclrtCreateStream](https://gitcode.com/cann/runtime/blob/master/docs/zh/api_ref/06_stream_management.md#aclrtCreateStream)，创建x、y、t、z对应的Tensor并准备输入数据。

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
int BeginScopeWithAclnn(
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
