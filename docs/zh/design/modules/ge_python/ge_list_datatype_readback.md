# 读取 LIST_DATA_TYPE 属性返回普通整数，无法作为 DataType 列表回写

## 行为

数据类型列表读取返回 DataType 枚举；可通过已有 set_list_data_type 回写，普通 int/float/bool 列表语义不变。

## 根因与修改

共用 list getter 没有按 VT_LIST_DATA_TYPE 将原生枚举数字恢复为 Python DataType，标量 getter 已执行此转换。

仅在 LIST_DATA_TYPE 分支转换每项为 DataType，仍在 finally 释放原生数组。

## 验证和边界

`tests/ge/ut/ge/graph/pyge_tests/attr_datatype_readback_cpu_test.py`：5 passed in 0.06s。测试执行实际生产 Python 实现，边界替身范围见测试。未验证真实 CANN/NPU 与全量 C++ UT/ST。

## 缓存封装

类型缓存的读取和更新由 `_AttrValue.get_or_cache` 在所属类内完成；装饰器只转发 getter、预期类型和参数，避免从类外直接访问受保护的缓存成员。缓存命中、类型不匹配、返回 `None`、异常和 setter 成败时的原有行为保持一致。

2026-10-10：Windows 与 Linux 均执行现有 5 项 CPU 回归并通过；缓存封装前后另做 24 组对照，返回值、getter 调用及缓存状态一致。此对照只替换 graph C API 边界，未验证真实设备。

缓存辅助方法按项目要求排在 `__repr__` 后。getter 异常直接向调用方传播，只有成功取值后才更新缓存和类型。
