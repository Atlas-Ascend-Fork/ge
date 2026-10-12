# TensorFlow入图

TensorFlow场景与PyTorch场景的主要区别在于：Adapter可从TF op\_def自动生成GE算子原型，开发者无需手写REG\_OP。两侧交付件简要示意图如下所示，TensorFlow场景样例可参考[Triton入图](../appendix/triton_into_graph.md)或[triton\_add\_custom](https://gitcode.com/cann/ge/tree/master/examples/custom_op/triton_add_custom)。

![](../figures/tf_into_graph.png)

下面将详细介绍各个阶段及其交付件。

1. **阶段1：算子定义**

    算子定义阶段声明算子的输入、输出、属性等元信息，供后续阶段使用。

    | 前端框架 | 定义方式 | 交付件 | 说明 |
    | --- | --- | --- | --- |
    | TensorFlow | TF op_def | libcustom_ops.so | TF侧算子声明，GE自动生成REG_OP |

    **TensorFlow侧算子注册示例（以TF1.15为例）：**

    ```c++
    #include "tensorflow/core/framework/op.h"
    #include "tensorflow/core/framework/op_kernel.h"
    #include "tensorflow/core/framework/shape_inference.h"
    #include "tensorflow/core/framework/common_shape_fns.h"

    using namespace tensorflow;

    // 通过TensorFlow提供的REGISTER_OP接口完成算子原型的注册
    // 注册后GE Adapter可自动从此定义生成GE算子原型，无需手写REG_OP
    REGISTER_OP("AddCustom")
        .Input("x: T")
        .Input("y: T")
        .Output("z: T")
        .Attr("T: {float, double, int32}")
        .SetShapeFn(shape_inference::BroadcastBinaryOpShapeFn);

    // TensorFlow自定义算子的CPU实现（不支持CPU，仅声明）
    // 实际计算在NPU上通过GE图执行完成
    class AddCustomOp : public OpKernel {
    public:
        explicit AddCustomOp(OpKernelConstruction* context) : OpKernel(context) {}
        void Compute(OpKernelContext* context) override {
            OP_REQUIRES(context, false, errors::Unimplemented("AddCustomOp is not supported on CPU"));
        }
    };

    // 注册CPU Kernel实现（必需，否则TF无法加载算子）
    REGISTER_KERNEL_BUILDER(Name("AddCustom").Device(DEVICE_CPU), AddCustomOp);
    ```

2. **阶段2：前端接入**

    将前端算子类型映射到GE算子类型，并处理属性转换。

    | 前端框架 | 接入方式 | 涉及交付件 | 关键点 |
    | --- | --- | --- | --- |
    | TensorFlow | Adapter自动映射 | npu_supported_ops.json | JSON配置算子可见性 |

    **TensorFlow场景需要：**

    1. TensorFlow侧自定义算子.so（声明算子可见）。
    2. GE侧交付件.so（只需REG\_AUTO\_MAPPING\_OP，无需额外提供REG\_OP）。
    3. npu\_supported\_ops.json（TensorFlow场景通过Adapter自动映射，该交付件为关键交付件）。

    **npu\_supported\_ops.json示例：**

    ```json
    {
      "custom_ops": [
        {
          "op_name": "AddCustom",
          "op_type": "AddCustom"
        }
      ]
    }
    ```

    **交付件组织示例：**

    ```tree
    custom_opp/
    ├── op_graph/
    │   └── lib/linux/x86_64/libcust_opapi.so  # GE交付件（只需REG_AUTO_MAPPING_OP）
    ├── framework/
    │   └── tensorflow/
    │       ├── libcustom_ops.so               # TF侧算子声明
    │       └── npu_supported_ops.json         # Adapter配置
    ```

    交付件统一规范请参见[交付件与OPP规范](deliverables_and_opp.md)。

3. **阶段3：图构建入图**

    图构建入图阶段将前端计算图转换为GE计算图。

    | 前端框架 | 入图路径 | 转换机制 |
    | --- | --- | --- |
    | TensorFlow | TF Graph → GE Graph | Adapter |

    此阶段由框架自动完成，无需开发者提供额外交付件。入图完成后，GE图中已包含自定义算子节点。

4. **阶段4：图编译**

    图编译的回调接口在GE编译期间触发，用于Shape推导、kernel编译和序列化。**回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
    | --- | --- | --- |
    | InferShape/InferDataType | 编译期Shape/DataType推导 | 所有场景（可选） |
    | Compile | 编译期RTC编译kernel | 在线编译/离线OM场景 |
    | Serialize | ATC编译期写入OM | 离线OM场景 |

    回调接口详细示例请参见[回调接口详解](callback_interface.md)。

5. **阶段5：图执行**

    该阶段是自定义算子的核心，GE在执行时回调Execute接口。

    **回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
    | --- | --- | --- |
    | Execute | 运行时执行kernel | 所有场景（必须实现） |
    | Deserialize | OM加载时恢复binary | 离线OM场景 |
    | UpdateHostArgs | I/O地址变化时刷新 | 静态图下沉场景 |

    回调接口详细示例请参见[回调接口详解](callback_interface.md)。

**总结：TensorFlow入图阶段与交付件对照表**

| 阶段 | 涉及交付件 | 说明 |
| --- | --- | --- |
| 阶段1：算子定义 | framework/tensorflow/libcustom_ops.so | TF侧算子声明 |
| 阶段2：前端接入 | framework/tensorflow/npu_supported_ops.json | Adapter配置 |
| 阶段3：图构建入图 | 无 | Adapter自动转换 |
| 阶段4：图编译 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |
| 阶段5：图执行 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |
