# GE原生构图入图

GE原生构图是最直接的入图方式，无需框架适配层，直接调用GE API创建算子节点。GE原生构图特点：

- 直接调用API构图，无框架转换层。
- op type与REG\_AUTO\_MAPPING\_OP注册名一致。
- 需手写REG\_OP proto头文件。

涉及各阶段交付件如下所示：

1. **阶段1：算子定义**

    算子定义阶段声明算子的输入、输出、属性等元信息，供后续阶段使用。

    **按前端分类的实现方式：**

    | 前端框架 | 定义方式 | 交付件 | 说明 |
    | --- | --- | --- | --- |
    | GE原生 | REG_OP头文件 | op_graph/include/*.h | 手写proto定义 |

    **GE原生REG\_OP示例：**

    ```c++
    // op_graph/include/add_custom.h
    REG_OP(AddCustom)
        .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16}))
        .INPUT(y, TensorType({DT_FLOAT, DT_FLOAT16}))
        .OUTPUT(z, TensorType({DT_FLOAT, DT_FLOAT16}))
        .OP_END_FACTORY_REG(AddCustom);
    ```

2. **阶段2-3：前端接入与构图**

    无框架转换层，直接调用API构图，示例如下：

    ```c++
    // 通过GE API直接创建自定义算子节点
    // "add_node"为节点名称，"AddCustom"为算子类型（与REG_OP一致）
    auto op = ge::OperatorFactory::CreateOperator("add_node", "AddCustom");
    // 设置算子输入端口
    op.SetInput("x", input_x);  // 将input_x连接到算子的x输入端口
    op.SetInput("y", input_y);  // 将input_y连接到算子的y输入端口
    // 将算子节点添加到计算图中
    graph.AddOp(op);
    ```

    构图侧使用的op type（本例AddCustom）必须与REG\_OP、REG\_AUTO\_MAPPING\_OP保持一致

3. **阶段4：图编译**

    图编译的回调接口在GE编译期间触发，用于Shape推导、kernel编译和序列化。**回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
    | --- | --- | --- |
    | InferShape/InferDataType | 编译期Shape/DataType推导 | 所有场景（可选） |
    | Compile | 编译期RTC编译kernel | 在线编译/离线OM场景 |
    | Serialize | ATC编译期写入OM | 离线OM场景 |

    回调接口详细示例请参见[回调接口详解](callback_interface.md)。

4. **阶段5：图执行**

    该阶段是自定义算子的核心，GE在执行时回调Execute接口。

    **回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
| --- | --- | --- |
| Execute | 运行时执行kernel | 所有场景（必须实现） |
| Deserialize | OM加载时恢复binary | 离线OM场景 |
| UpdateHostArgs | I/O地址变化时刷新 | 静态图下沉场景 |

    回调接口详细示例请参见[回调接口详解](callback_interface.md)。

**总结：GE原生构图阶段与交付件对照表**

| 阶段 | 涉及交付件 | 说明 |
| --- | --- | --- |
| 阶段1：算子定义 | op_graph/include/*.h | REG_OP proto头文件 |
| 阶段2：前端接入 | 无 | 直接API调用 |
| 阶段3：图构建入图 | 无 | 直接构图 |
| 阶段4：图编译 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |
| 阶段5：图执行 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |
