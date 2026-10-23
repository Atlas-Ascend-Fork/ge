# GraphBuilder.set_graph_output 接受其他 builder 的 TensorHolder，原生调用指向错误所属图

## 行为与根因

设置图输出时 TensorHolder 必须属于当前 builder；外来输出在调用原生接口之前抛 ValueError，正常同图输出仍可设置及 build。

只检查类型和 receiver 的可用性，未检查 tensor.get_owner_builder()；原生 EsSetGraphOutput 按 tensor 归属定位图，不能验证 Python receiver。

在 set_graph_output 中核对 tensor.get_owner_builder() is self；build_and_reset(outputs=...) 自动复用同一检查。

## 验证和边界

回归 `tests/ge/ut/ge/graph/pyge_tests/graph_output_owner_cpu_test.py`：4 passed in 0.07s。执行真实 GraphBuilder、TensorHolder、Graph、枚举及 ctypes 缓冲，原生库边界使用替身。未验证 CANN/NPU 或全量 C++ UT/ST。

## 原生参数封装

TensorHolder 的只读 ctypes `_as_parameter_` 协议提供借用指针。GraphBuilder 的六种 tensor/node 属性设置接口直接传入 TensorHolder，由 ctypes 按已有指针类型转换，不跨类读取 `_handle`，也不转移指针所有权。公共方法签名及 C API 不变。CPU 回归使用真实类型指针和 CFUNCTYPE 回调检查地址、UTF-8 名称及值、成功/失败返回；不代表真实 GE 原生库或 NPU 验证。

GraphBuilder 与 TensorHolder 的协作统一使用 `apply_scope_infos_to_node` 方法，将当前属性和控制依赖应用到新建节点。四种算术结果不再从类外调用受保护方法；节点处理顺序、返回对象及 owner 引用保持原样。

保留原 _apply_scope_infos_to_node 内部转发入口，使已有算子生成器及此前生成的 Python 包装代码继续可用；该入口只委托统一实现，不重复应用属性或控制边。
