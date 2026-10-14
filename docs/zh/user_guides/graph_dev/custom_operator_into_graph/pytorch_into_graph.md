# PyTorch入图

本章以**PyTorch+TorchAir**为前端框架，详细描述自定义算子入GE图的完整流程。

## 前提条件

参见[环境准备](env_preparation.md)完成环境搭建。

## 入图阶段

自定义算子入图整体分为5个渐进式阶段，开发者按顺序完成各阶段交付件即可实现入图。两侧交付件简要示意图如下所示：

![](../figures/pytorch_into_graph.png)

下面将详细介绍各个阶段及其交付件。

1. **阶段1：算子定义**

    算子定义阶段声明算子的输入、输出、属性等元信息，供后续阶段使用。在PyTorch场景下，需要**两侧并行定义**：

    | 前端框架 | 定义方式 | 交付件 | 说明 |
    | --- | --- | --- | --- |
    | GE侧 | REG_OP头文件 | op_graph/include/*.h | 手写proto定义 |
    | PyTorch侧 | TORCH_LIBRARY | python/*.py | 定义schema（函数签名） |

    - **GE侧REG\_OP示例：**

        ```c++
        // 注册GE算子原型：定义算子的输入、输出和属性
        // op_graph/include/add_custom.h
        REG_OP(AddCustom)
            .INPUT(x, TensorType({DT_FLOAT, DT_FLOAT16}))    // 声明输入x
            .INPUT(y, TensorType({DT_FLOAT, DT_FLOAT16}))    // 声明输入y
            .OUTPUT(z, TensorType({DT_FLOAT, DT_FLOAT16}))   // 声明输出z
            .OP_END_FACTORY_REG(AddCustom);
        ```

    - **PyTorch侧TORCH\_LIBRARY示例：**

        ```python
        # python/add_custom.py
        import torch

        # 定义算子schema，需与REG_OP的输入输出一致
        torch.library.define("my_ops", "add_custom(Tensor x, Tensor y) -> Tensor")
        ```

        **一致性要求**：PyTorch schema中的输入/输出数量和类型必须与REG\_OP定义严格一致，否则图转换时会报错。

2. **阶段2：前端接入**

    前端接入阶段将PyTorch FX节点映射到GE算子类型。PyTorch场景通过**TorchAir converter**机制实现，需要两侧交付件配合：

    **Python侧需要：**

    1. TORCH\_LIBRARY定义算子schema。
    2. TORCH\_LIBRARY\_IMPL注册PrivateUse1（NPU执行）、Meta（shape推导）、XLA（图编译）等key。
    3. @torchair.register\_fx\_node\_ge\_converter将PyTorch FX节点映射到GE算子类型。

    **Python侧接入代码：**

    ```python
    # python/add_custom_converter.py
    import torch
    import torchair

    # 定义算子schema
    torch.library.define("my_ops", "add_custom(Tensor x, Tensor y) -> Tensor")

    # 注册实现（PrivateUse1=NPU执行）
    @torch.library.impl("my_ops.add_custom", "PrivateUse1")
    def add_custom_impl(x, y):
        return torch.empty_like(x)  # placeholder，实际由GE执行

    # 注册GE converter：将FX节点映射到GE算子
    @torchair.register_fx_node_ge_converter(torch.ops.my_ops.add_custom.default)
    def convert_add_custom(x, y, meta_outputs=None):
        return torchair.ge.custom_op(
            "AddCustom",           # GE op type（必须与REG_AUTO_MAPPING_OP一致）
            inputs={"x": x, "y": y},
            outputs=["z"]
        )
    ```

    > [!NOTE]说明
    >**converter签名约定**：位置参数依次对应算子输入（x、y），meta\_outputs 为 TorchAir 传入的编译期输出规格。**不要**把输出当作位置参数写在签名中。

    **交付件组织示例：**

    ```tree
    custom_opp/
    ├── op_graph/
    │   ├── include/add_custom.h    # REG_OP
    │   └── lib/linux/x86_64/libcust_opapi.so  # GE交付件
    └── python/
        ├── add_custom.py           # TORCH_LIBRARY定义
        └── add_custom_converter.py # converter注册
    ```

    交付件统一规范请参见[交付件与OPP规范](deliverables_and_opp.md)。

3. **阶段3：图构建入图**

    构建入图阶段将PyTorch FX Graph自动转换为GE计算图。

    | 前端框架 | 入图路径 | 转换机制 |
    | --- | --- | --- |
    | PyTorch | FX Graph → GE Graph | TorchAir自动转换 |

    此阶段由TorchAir自动完成，**无需开发者提供额外交付件**。入图完成后，GE图中已包含自定义算子节点。

4. **阶段4：图编译**

    图编译的回调接口在GE编译期间触发，用于Shape推导、kernel编译和序列化。**回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
    | --- | --- | --- |
    | InferShape/InferDataType | 编译期Shape/DataType推导 | 所有场景（可选） |
    | Compile | 编译期RTC编译kernel | 在线编译/离线OM场景 |
    | Serialize | ATC编译期写入OM | 离线OM场景 |

    签名、上下文 API、典型流程请参见[回调接口详解](callback_interface.md)。

5. **阶段5：图执行**

    该阶段是自定义算子的核心，GE在执行时回调Execute接口。

    **回调接口与触发时机：**

    | 回调接口 | 触发时机 | 适用场景 |
    | --- | --- | --- |
    | Execute | 运行时执行kernel | 所有场景（必须实现） |
    | Deserialize | OM加载时恢复binary | 离线OM场景 |
    | UpdateHostArgs | I/O地址变化时刷新 | 静态图下沉场景 |

    详细示例请参见[样例解析](quick_start.md#样例解析)。

**总结：PyTorch入图阶段与交付件对照表：**

| 阶段 | 涉及交付件 | 说明 |
| --- | --- | --- |
| 阶段1：算子定义 | op_graph/include/.h + python/.py | proto头文件等 |
| 阶段2：前端接入 | python/*_converter.py | 解析插件.so等 |
| 阶段3：图构建入图 | 无 | 无额外交付件（由TorchAir自动转换） |
| 阶段4：图编译 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |
| 阶段5：图执行 | op_graph/lib/<os\>/<arch\>/*.so | 回调实现在GE交付件中 |

## 工程编译与部署

CMake构建关键要素如下：

```cmake
# 头文件路径（必须包含）
target_include_directories(my_op PRIVATE
    ${ASCEND_HOME_PATH}/include
    ${ASCEND_HOME_PATH}/include/graph
    ${ASCEND_HOME_PATH}/include/register
    ${ASCEND_HOME_PATH}/include/external
)

# 链接库（按需选择）
target_link_libraries(my_op PRIVATE
    ascendcl acl_rt ge_compiler ge_executor
)

# 输出.so
add_library(my_op SHARED custom_op.cpp)

```

**PyTorch场景额外依赖：**  需链接TorchNPU、c10、lowering等，并包含TorchNPU头文件路径。构建产物需按OPP包规范安装，如下所示：

```tree
output/                              # ASCEND_CUSTOM_OPP_PATH指向此目录
├── op_graph/
│   ├── include/my_op.h              # proto头文件（供构图侧使用）
│   └── lib/linux/x86_64/
│       ├── libcust_opapi.so         # 交付件
│       └── my_kernel.cpp            # kernel 源码（RTC场景需要）
└── bin/
    └── my_kernel.bin                # kernel binary（按需）
```

完整运行步骤请参见[快速开始](quick_start.md)。
