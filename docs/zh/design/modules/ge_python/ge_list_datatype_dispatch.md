# Python 属性列表的 DataType 枚举被错误写入 list_int

## 行为约束

完整 DataType 列表写入 LIST_DATA_TYPE，普通 int/bool/float/string 列表保留既有类型。

## 根因与修复

DataType 继承 IntEnum，也是 int；set_value 的列表分派先检查 int 再检查 DataType，后者无法到达。

将完整 DataType 列表的分支移动到 int 列表之前，沿用已有类型校验和 C API。

## 影响和验证边界

Node、Graph、Operator 属性中的数据类型列表变成整数列表，原生读取 LIST_DATA_TYPE 属性时发生类型不匹配。

`tests/ge/ut/ge/graph/pyge_tests/attr_list_datatype_dispatch_test.py` 验证实际 Python 类型分派和 C 调用入参，仅替换原生边界。结果：5 passed in 0.06s。没有进行真实 CANN 设备集成验证。

## 缓存封装

类型缓存的命中判断和成功更新在 `_AttrValue.get_or_cache` 内完成，装饰器只转发 getter、预期类型和参数，避免从类外访问受保护成员。辅助方法排在 `__repr__` 后；getter 异常自然向上传播，失败时不更新缓存。DataType 列表写入分派及原有 setter/getter 接口保持不变。
