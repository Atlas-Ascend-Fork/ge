---
name: om2-abi-compat
description: 仅用于 OM2 模型 SO 与 Executor 间（C ABI）及 OM2 落盘格式的兼容性变更或评估，覆盖 inc/framework/om2/ 目录下公开头文件（model_api/om2_model_api.h、model_data/gert_model_data.h 等）、GertModel* 导出 API/回调、OM2 dlsym 查询及其跨边界数据结构，以及 OM2 特性清单与版本宏（inc/framework/common/om2_feature_list.h 的 kOm2FeatureList/Om2FeatureIndex、om2_package_contants.h 的 OM2_FEATURE_* 常量、om2_package_helper.cc 的探测表 kOm2DetectByIndex、gert_model_data.h 的 GERT_EXECUTOR_VERSION，含"新增或不兼容变更必须抬版本宏"的检查）；不用于普通 dlsym、回调、代码生成、内部结构体或非 OM2 接口。
---

# OM2 ABI 兼容性

## 规则来源

作出兼容性判断前，先阅读 [OM2 兼容性规则](../../../inc/framework/om2/README.md)。该文件是兼容规则的唯一来源；本 skill 只定义开发工作流。

## 使用时机

用户显式指定 `$om2-abi-compat`，或要求评估 OM2 ABI 兼容性时使用。

以下变更也应自动使用：

- `inc/framework/om2/` 目录（含 `model_api/om2_model_api.h`、`model_data/gert_model_data.h` 等公开头文件）或其他 SO 与 Executor 之间的公开 ABI 头文件；
- OM2 落盘格式：`model_data/om2_package_contants.h` 路径常量及各 JSON 字段名与取值语义（README 第 9 节）；
- SO 导出 API、回调 typedef 或 Executor 的 `dlsym` 查询；
- 生成此类 ABI 的 OM2 代码生成模板；
- 跨该边界的结构体、枚举、flag、句柄、字符串或所有权约定。
- OM2 特性清单与版本宏：`inc/framework/common/om2_feature_list.h`特性清单、
  `model_data/om2_package_contants.h` 的 `OM2_FEATURE_*` 特性名称常量、`base/common/helper/om2_package_helper.cc` 的特性探测驱动表
  `kOm2DetectByIndex`、`model_data/gert_model_data.h` 的 `GERT_EXECUTOR_VERSION` 执行器版本号。

不影响 ABI 边界或落盘格式兼容性的普通内部重构不使用。

## 工作流

1. **分类变更。** 阅读规则来源、公开头文件、相关生成器源码（而非仅生成结果）和当前 diff，说明是否影响已发布 ABI，以及属于新增还是修改已有项。
2. **选择兼容形态。** 不得改变已发布符号或已有结构体成员。新增能力优先采用新的导出 API，或向可扩展输入/输出结构体尾部追加字段；新 API 使用独立的输入和输出结构体。
3. **明确边界契约。** 记录参数方向、尾字段缺失时的默认行为、字段存在性/有效性语义、所有权与生命周期；涉及回调时还应记录时机、线程/重入、状态码和字符串编码等约定。
4. **在生成源处实现。** 若生成器负责公开声明，修改生成器而非仅修改生成结果；不得为了满足新规则而改造冻结的既有 ABI。
5. **补充必要兼容性 UT。** 公开结构体更新 `tests/ge/ut/ge/compatibility/om2/struct_layout_check.cpp`；公开枚举更新 `enum_value_check.cpp`。结构体首次尾部扩展时，在 `struct_size_check.cpp` 增加其旧尺寸行为用例。仅在新增公开数值常量或落盘路径常量时添加 `const_check.cpp`。
6. **登记特性即抬升版本宏。** 新增必需能力提高 `GERT_EXECUTOR_VERSION` 的 minor 版本号，落盘结构不兼容变更提高 major 版本号，登记版本写成本次抬升后的版本值。四处修改必须同一次提交完成——特性名常量（`om2_package_contants.h` 的 `OM2_FEATURE_*`）、特性索引编号（`om2_feature_list.h` 的 `Om2FeatureIndex`，追加在 `kFeatureIndexMax` 之前）、探测函数驱动表（`om2_package_helper.cc` 的 `kOm2DetectByIndex`，下标与特性索引编号一致）、特性清单条目（`om2_feature_list.h` 的 `kOm2FeatureList`）；新增特性名常量时在 `const_check.cpp` 补取值冻结用例。

## 硬阻断

若变更会导致以下任一情况，停止修改并提出兼容替代方案：

- 删除、改名或变更已发布导出 API 或回调签名；
- 删除、改名、改类型或移动已发布结构体成员；
- 字段未尾部追加、复用 `abi_pad_N`，或改变 packing/对齐；
- 可扩展结构体的变化经由 ABI 边界的按值对象传播，且变化对象之前没有指针间接；
- 新增跨 ABI 标量的位宽不明确；
- 为已有 API 增加位置参数，而非新增 API 或使用可扩展输入/输出结构体。
- 特性清单新增条目或变更既有条目，而 `GERT_EXECUTOR_VERSION` 未在同一次提交提高；
- 将已登记特性的版本值改小；
- 把兼容类变更（bug 修复、DFX、性能优化）登记进 `used_features`；

## 完成输出

输出 ABI 变更分类、边界契约、修改的生成器/头文件/实现位置、必要兼容性 UT、未决语义问题，以及特性变更名称和版本号变化情况。未补齐必要兼容性 UT 时，不得宣称 ABI 工作完成。
