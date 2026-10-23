# TensorDesc 超出 int64 范围的维度被 ctypes 静默截断

## 行为

原生 int64 无法表示的维度在构造 ctypes 数组前抛 ValueError，设备入口不调用；int64 边界、0 和现有动态维度 -1/-2 保留。

## 根因与修改

_normalize_dims 只检查 Python int 类型，ctypes.c_int64 接受大整数并按 64 位截断。

共享维度归一化加入有符号 64 位范围校验，覆盖 Shape/TensorDesc 的构造和两个 shape setter。

## 验证和边界

`tests/ge/ut/ge/graph/pyge_tests/tensor_desc_dimension_range_cpu_test.py`：11 passed in 0.07s。测试执行实际生产 Python 实现，边界替身范围见测试。未验证真实 CANN/NPU 与全量 C++ UT/ST。
