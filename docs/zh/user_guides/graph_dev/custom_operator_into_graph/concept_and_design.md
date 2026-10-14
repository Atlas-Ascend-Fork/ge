# 概念与设计

## 接口体系

所有自定义算子实现类均需继承自BaseCustomOp，并根据需求灵活组合以下能力接口，每个接口对应一个独立的回调时机，GE在**图编译或执行的特定阶段**调用对应回调。

```tree
BaseCustomOp（公共基类）
├── EagerExecuteOp     → Execute(ctx)          运行时执行
├── ShapeInferOp       → InferShape(ctx)       Shape推导
│                        → InferDataType(ctx)    DataType推导
├── CompilableOp       → Compile(ctx)          在线编译
├── PortableOp         → Serialize(buf)        序列化到OM
│                        → Deserialize(buf)      从OM反序列化
└── ArgsUpdater        → UpdateHostArgs(ctx)   I/O地址刷新
```

接口详细介绍请参见[回调接口详解](callback_interface.md)。

实现类示例：

```c++
// 继承EagerExecuteOp获得运行时执行能力
class MyCustomOp : public EagerExecuteOp {
  graphStatus Execute(gert::EagerOpExecutionContext *ctx) override { /* ... */ }
};

// 注册到GE：图中遇到"MyCustomOp"类型节点时实例化此类并回调Execute
REG_AUTO_MAPPING_OP(MyCustomOp);
```

## 注册机制

自定义算子需要两类注册：

- **REG\_OP**  ：**声明算子原型**

    定义算子的输入、输出和属性proto，供构图侧创建节点时使用：

    ```c++
    // 注册GE算子原型，声明算子的输入、输出及支持的数据类型
    // MyCustomOp为算子类型名，需与后续REG_AUTO_MAPPING_OP的类名保持一致
    REG_OP(MyCustomOp)
        .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16}))    // 输入x
        .INPUT(y, TensorType({DT_FLOAT, DT_FLOAT16}))    // 输入y
        .OUTPUT(z, TensorType({DT_FLOAT, DT_FLOAT16}))   // 输出z
        .OP_END_FACTORY_REG(MyCustomOp);
    ```

- **REG\_AUTO\_MAPPING\_OP**：**映射实现类**

    将C++实现类映射到op type，GE按op type名查找并实例化：

    ```c++
    // 自定义算子实现类，继承EagerExecuteOp以获得运行时执行能力
    class MyCustomOp : public EagerExecuteOp { /* ... */ };
    // 将实现类注册到GE，GE在图中遇到"MyCustomOp"类型的节点时，会实例化此类并回调其Execute方法
    REG_AUTO_MAPPING_OP(MyCustomOp);
    ```

    > [!NOTE]说明
    >**命名约束**：REG\_OP中的op type名、REG\_AUTO\_MAPPING\_OP的类名、构图侧使用的op type必须三者一致。

- **两种Shape推导范式（选一种）**

  | 范式 | 写法 | 适用 |
  | --- | --- | --- |
  | 继承接口 | 实现类继承ShapeInferOp，覆写InferShape(ctx) | 与EagerExecuteOp等接口组合使用，面向对象风格。 |
  | 注册函数 | IMPL_OP(MyCustomOp).InferShape(InferShapeFn) | 与实现类解耦，独立注册推导函数。 |

    两种方式皆可，选定其一贯穿整个工程即可，差异仅在于“将推导逻辑封装于实现类内部”还是“注册为独立函数”。

## 交付件与OPP包

自定义算子以.so交付件形式被GE加载，通过ASCEND\_CUSTOM\_OPP\_PATH环境变量指定路径。推荐的OPP包目录结构，.so命名部署步骤请参见[交付件与OPP规范](deliverables_and_opp.md)。

配置方式：

```bash
export ASCEND_CUSTOM_OPP_PATH="<custom_opp_root>:$ASCEND_CUSTOM_OPP_PATH"
```

## 执行场景介绍

自定义算子的开发需依据具体场景实现不同的接口组合，本节将介绍典型应用场景及其开发要点，**执行场景**决策表如下：

| 场景 | 说明 | 接口组合 | 典型链路 | 参考样例 |
| --- | --- | --- | --- | --- |
| 场景1：动态图在线执行 | kernel预编译好，Execute直接launch | EagerExecuteOp+ShapeInferOp（可选） | 构图 → 直接执行 | [ascendc_add_custom](../../../../../examples/custom_op/ascendc_add_custom/README.md) |
| 场景2：在线编译+在线执行 | 图编译期RTC编译kernel | EagerExecuteOp+CompilableOp+ShapeInferOp(可选) | 构图 → Compile → 执行 | - |
| 场景3：离线OM模型下沉 | 编译产物序列化进OM，加载后恢复 | EagerExecuteOp+CompilableOp+ShapeInferOp+PortableOp | AIR → ATC → OM → ACL | [compilable_add_custom](../../../../../examples/custom_op/compilable_add_custom/README.md) |
| 场景4：数据依赖shape | 输出shape 依赖运行期数据，回写shape buffer | EagerExecuteOp+ShapeInferOp | 构图 → 执行+shape buffer回写 | [data_dependent_shape_custom](../../../../../examples/custom_op/data_dependent_shape_custom/README.md) |

- **场景1：动态图在线执行**

    最简单的场景。kernel已预编译好（Ascend C .asc同库编译、Triton .npubin等），只需在Execute中加载并launch。

    ```plaintext
    构图 → GE回调Execute → 获取输入 → 分配输出 → launch kernel → 返回
    ```

    **关键点：**

  - 只需实现EagerExecuteOp。
  - ShapeInferOp可选：如果输出shape与输入相同且REG\_OP已声明，可省略。
  - kernel binary随.so一起编译，或在Execute中从文件加载。

- **场景2：在线编译+在线执行**

    kernel源码需要在GE编译阶段通过RTC（Runtime Compilation）编译为device binary。

    ```plaintext
    构图 → GE回调Compile（RTC编译kernel）
         → GE回调Execute（加载binary，launch kernel）
    ```

    **关键点：**

  - Compile回调中可通过ctx-\>GetInputTensor\(i\) 获取输入的shape、dtype、format等完整元信息。
  - 建议按shape生成binary key，支持多shape/多kernel管理。
  - 编译产物缓存在实现类的成员变量中，供Execute使用。

- **场景3：离线OM模型下沉**

    在场景2基础上，需要将编译产物序列化到OM模型文件中，后续加载OM时反序列化恢复。

    ```plaintext
    构图 → ATC离线编译  → 回调Compile（RTC编译）
                         → 回调Serialize（binary写入OM）
         → acl加载OM    → 回调Deserialize（从OM恢复binary）
                         → 回调Execute（launch kernel）
    ```

    **关键点：**

  - 必须实现PortableOp，Serialize/Deserialize的buffer格式由用户自定义，GE只透传不解析。
  - ShapeInferOp在OM编译阶段被调用，用于推导输出shape/dtype。
  - 支持多份binary的序列化（按key管理）。

- **场景4：数据依赖shape（三类算子）**

    输出shape依赖运行时输入数据（如Where、NonZero），编译期只能确定上界。

    ```plaintext
    InferShape → 输出shape上界（含unknown维）
    Execute    → 按上界分配输出+分配shape buffer
               → launch kernel（kernel写回实际shape到shape buffer）
               → stream sync → D2H读回shape buffer → 更新输出shape
    ```

    **关键点：**

  - InferShape中用-1表示unknown维。
  - Execute中按最大shape分配输出，额外分配shape buffer（通过MallocWorkSpace）。
  - kernel执行后需自行同步stream并读回shape buffer。
  - shape buffer格式：\[ndim, d0, d1, ...\]（uint64\_t数组）。

## 广播语义示例

Add类算子的输出Shape推导遵循广播规则：从尾部对齐维度，若维度为1则扩展以匹配另一侧，若相等则保留，其他情况视为非法。

```c++
graphStatus InferShapeForAdd(gert::InferShapeContext *context) {
  const auto &in0 = *context->GetInputShape(0);
  const auto &in1 = *context->GetInputShape(1);
  auto *out = context->GetOutputShape(0);

  size_t dims0 = in0.GetDimNum();
  size_t dims1 = in1.GetDimNum();
  size_t out_dims = std::max(dims0, dims1);
  out->SetDimNum(out_dims);

  for (size_t i = 0; i < out_dims; ++i) {
    // 尾部对齐：超出自身维度的部分视为维度1
    int64_t d0 = (i + dims0 >= out_dims) ? in0.GetDim(i - (out_dims - dims0)) : 1;
    int64_t d1 = (i + dims1 >= out_dims) ? in1.GetDim(i - (out_dims - dims1)) : 1;

    if (d0 == d1 || d0 == 1 || d1 == 1) {
      out->SetDim(i, std::max(d0, d1));   // 维度为1时扩展为另一侧维度
    } else {
      // 非法广播：如(2)与(3)，不可逐维取max
      return GRAPH_FAILED;
    }
  }
  return GRAPH_SUCCESS;
}

IMPL_OP(AddCustom).InferShape(InferShapeForAdd);
```
