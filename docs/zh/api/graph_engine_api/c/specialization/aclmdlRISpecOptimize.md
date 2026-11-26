# aclmdlRISpecOptimize

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
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/specialization/aclmdlRISpecOptimize_res.md#id1 -->

## 头文件

\#include <aclmodel\_specialization.h\>

## 功能说明

对模型中的核函数（Kernel）进行常量化编译，并将编译结果更新到原模型中。

本接口遍历整个模型的Kernel。若只需对指定范围进行常量化编译，请使用[aclmdlRISpecOptimizeByFlag](aclmdlRISpecOptimizeByFlag.md)。

## 函数原型

```cpp
aclError aclmdlRISpecOptimize(aclmdlRI modelRI, aclmdlRISpecOptions *options)
```

## 参数说明

**表1**  接口参数说明

| 参数名 | 输入/输出 | 描述 |
| :--- | :---: | :--- |
| modelRI | 输入 | 待常量化的模型句柄，不能为空。可通过[aclmdlRICaptureEnd](https://gitcode.com/cann/runtime/blob/9.3.0-beta.2/docs/zh/api_ref/15_model_running_instance_management.md#aclmdlRICaptureEnd)获取。本接口直接更新该模型，不创建新模型。 |
| options | 输入 | 常量化编译配置。传入nullptr时，不追加用户编译选项，不启用SuperKernel编译，按串行方式编译。各成员的配置要求请参见[表2](#table-aclmdl-ri-spec-options)。 |

### aclmdlRISpecOptions

相关结构体定义位于`"aclmodel_specialization.h"`，成员说明如下。

**表2**  aclmdlRISpecOptions成员说明<a id="table-aclmdl-ri-spec-options"></a>

| 参数名 | 描述 |
| :--- | :--- |
| jobs | 编译并行度预留配置。当前版本固定串行编译，本参数暂不生效。 |
| specCompileOptionCount | 普通编译参数规则的数量。为0时不配置普通编译参数规则。 |
| specCompileOptions | 指定各Kernel的普通编译参数。specCompileOptionCount非0时不能为空。<br>成员说明：<br>**kernelName**：用于匹配Kernel入口名称，并为匹配的Kernel追加编译选项。支持填写完整入口名称或匹配模式，区分大小写。匹配模式仅支持字母、数字、下划线、连字符、`.`和`*`，且不能以`*`开头。匹配模式中，`.`表示任意一个字符，`*`表示前一个字符重复零次或多次；例如，`Add.*`匹配以Add开头的Kernel。传入nullptr时匹配全部Kernel。<br>**compileOptionCount**：compileOptions数组的元素个数。为0时允许compileOptions为空。<br>**compileOptions**：编译选项字符串数组。compileOptionCount非0时数组不能为空，各字符串不能为空指针或空字符串。每个字符串作为一个完整参数传递，不拆分。具体选项须由下游编译器支持。 |
| enableSK | 是否启用SuperKernel编译。true表示启用，false表示禁用。options为nullptr时按false处理。 |
| specSKFeatureCount | specSKFeatures数组的元素个数。为0时不配置额外SuperKernel特性。 |
| specSKFeatures | SK编译特性配置，按Kernel入口名称指定调试及数据缓存维护相关编译选项。specSKFeatureCount非0时不能为空，仅在enableSK为true时生效。<br>成员说明：<br>**feature**：特性名称，不能为空指针或空字符串，区分大小写。当前处理的特性见[表3](#table-aclmdl-ri-spec-sk-features)。<br>**kernelNameCount**：kernelNames数组的元素个数。为0时不配置Kernel匹配名单。<br>**kernelNames**：Kernel入口名称匹配模式数组，匹配规则与kernelName相同。对当前处理的特性，kernelNameCount非0时数组不能为空，各模式不能为空指针或空字符串。 |
| reserved | 保留字段，须置为0。 |

多条普通编译参数规则同时命中时，按配置数组顺序追加全部参数，不覆盖、不去重。规则只配置编译参数，不限制模型中的常量化候选范围。

例如，先配置`Add.*`匹配规则并追加`-g`，再配置`AddCustom.*`匹配规则并追加`-fno-ident`，则同时匹配这两条规则的`AddCustom_0`将依次追加`-g`和`-fno-ident`。

**表3**  当前处理的SuperKernel特性<a id="table-aclmdl-ri-spec-sk-features"></a>

| 特性名称 | 功能说明及配置范围 |
| --- | --- |
| DEBUG_PER_OP_MAX_CORE_NUM | 为匹配的Kernel启用SuperKernel调试编译配置，用于生成Kernel调试相关代码。 |
| DCCI_DISABLE_ON_KERNEL | 为匹配的Kernel配置数据缓存维护（DCCI）禁用编译选项。kernelNameCount非0时，同时为全部候选Kernel配置GlobalTensor的GetValue/SetValue缓存维护编译选项。 |

匹配得到的SuperKernel编译选项按完整字符串去重，仅保留首次出现的参数。

## 返回值说明

返回值类型为aclError，ACL_SUCCESS表示优化流程完成，其他值表示失败。没有可常量化的Kernel，或所有Kernel均被跳过时，也可返回ACL_SUCCESS。

| 返回值 | 描述 |
| --- | --- |
| ACL_ERROR_INVALID_PARAM | 配置参数无效或模型句柄为空。 |
| ACL_ERROR_BAD_ALLOC | 内存分配失败或容器长度超限。 |
| ACL_ERROR_FAILURE | 发生其他异常。 |

## 约束说明

- 模型须已完成捕获并处于空闲状态。调用期间，调用方须保证没有对该模型的并发执行、修改或销毁。
- 同一模型仅允许使用aclmdlRISpecOptimize与[aclmdlRISpecOptimizeByFlag](aclmdlRISpecOptimizeByFlag.md)中的一个接口进行优化，且最多调用一次。
- options及其引用的数组、字符串在调用期间须保持有效且不变。数组的实际可读长度须与数量字段一致，字符串须以空字符结尾。
- Kernel须具有可用的常量化资源标识和参数信息，并能获取编译所需的参数值。不满足条件的Kernel保留原函数；单个Kernel的常量化编译失败时，记录告警并继续处理其他Kernel。
- 常量化编译成功的Kernel仅替换任务的函数句柄，保留原任务的其他下发参数。后续运行模型时，参与常量化的值须保持不变。
- 编译产物加载失败或找不到对应Kernel入口时，返回相应错误码，此时尚未修改模型。任务替换、模型更新或资源托管失败时，返回相应错误码，模型可能已被修改，且不进行回滚。调用失败时，可查看ERROR级别日志，定位失败阶段及相关模型或任务。
- 调用方取得模型句柄后，无论优化是否成功，均须在不再使用模型时调用[aclmdlRIDestroy](https://gitcode.com/cann/runtime/blob/9.3.0-beta.2/docs/zh/api_ref/15_model_running_instance_management.md#aclmdlRIDestroy)。优化成功后，常量化编译生成的Kernel资源随模型销毁自动释放。

## 调用示例

以下片段演示捕获两次`AddCustom`调用，并通过`aclmdlRISpecOptimize`对捕获的模型进行常量化编译。

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
int OptimizeModelWithAclnn(
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

    // 第二段：下发计算任务。
    ASCENDC_CHECK(aclnnAddCustom(workspace1, workspaceSize1, executor1, stream));
    ASCENDC_CHECK(aclnnAddCustom(workspace2, workspaceSize2, executor2, stream));

    ASCENDC_CHECK(aclmdlRICaptureEnd(stream, &modelRI));

    // 对捕获后的模型进行常量化编译。
    ASCENDC_CHECK(aclmdlRISpecOptimize(modelRI, nullptr));
    return 0;
}
```
