# 子类新增 @pattern 方法时静默丢失基类已声明的融合 pattern

## 行为约束

子类添加 decorated pattern 时保留未被覆盖的基类 pattern，按 Python MRO 处理同名覆盖与菱形继承；子类显式 legacy patterns() 的既有选择不改变。

## 根因与修复

子类生成新的 patterns() 适配器时，仅遍历 cls.__dict__，没有读取继承的方法；旧基类适配器又被覆盖。

生成适配器时按 MRO 收集可见的 decorated 方法，记录已经出现的属性名以尊重覆盖；是否在本类混用两种声明方式仍只检查本类定义。

## 影响和验证边界

通过继承复用并扩展 pattern 的融合 pass 静默减少可匹配的模式，导致应生效的优化遗漏。这里只验证 Python 方法发现和适配，不宣称已验证 NPU 优化性能。

`tests/ge/ut/ge/graph/pyge_tests/pattern_inheritance_cpu_test.py` 验证实际 Python 类型分派和 C 调用入参，仅替换原生边界。结果：6 passed in 0.08s。没有进行真实 CANN 设备集成验证。
