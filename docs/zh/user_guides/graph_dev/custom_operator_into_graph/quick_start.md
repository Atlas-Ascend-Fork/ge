# 快速开始

## 10分钟跑通最小示例

本样例以Add算子为例，展示如何将Ascend C自定义算子通过TorchAir接入GE图：GE交付件侧通过实现EagerExecuteOp完成图执行，Python侧同时支持eager直接调用与torch.compile图模式编译调用，简要流程如下：

```plaintext
PyTorch调用 → FX图 → TorchAir converter → GE图 → GE回调Execute → launch Ascend C kernel
```

完整样例代码请参见[ascendc\_add\_custom](../../../../../examples/custom_op/ascendc_add_custom/README.md)。

1. 获取示例

    ```bash
    git clone https://gitcode.com/cann/ge.git
    cd ge/examples/custom_op/ascendc_add_custom
    ```

    完整目录结构如下：

    ```tree
    ascendc_add_custom
    ├── CMakeLists.txt             // 编译libcust_opapi.so，配置PyTorch/TorchAir/CANN依赖
    ├── README.md
    ├── add_custom_kernel
    │   ├── add_custom.asc        // Ascend C kernel实现与PyTorch注册代码（TORCH_LIBRARY）
    │   ├── add_custom_kernel.h   // kernel声明
    │   └── custom_op.cpp         // GE交付件，REG_OP + Execute + InferShape
    └── script
        └── add_custom_test.py    // Python测试脚本（含converter注册，覆盖eager与图模式）
    ```

    - add\_custom\_kernel/add\_custom.asc：Ascend C Add kernel的实现文件，同时包含PyTorch自定义算子注册逻辑（TORCH\_LIBRARY\_FRAGMENT + TORCH\_LIBRARY\_IMPL）。
    - add\_custom\_kernel/custom\_op.cpp：自定义算子的GE交付件，实现Execute路径、REG\_OP算子原型注册、InferShape推导并完成GE入图。
    - script/add\_custom\_test.py：Python测试脚本，覆盖eager执行和TorchAir图模式编译执行，同时包含converter注册。
    - CMakeLists.txt：编译libcust\_opapi.so并配置PyTorch、TorchAir、CANN依赖。

2. 构建

    执行如下命令构建工程：

    ```bash
    mkdir -p build
    cd build
    cmake ..
    make -j"$(nproc)"
    ```

    编译完成后，在build路径会生成自定义算子交付件libcust\_opapi.so。

3. 部署

    ```bash
    # 回到示例根目录
    cd ..

    # 按OPP规范组织输出目录（示例仓CMakeLists若配置了install规则，可改用：cmake --install build）
    mkdir -p output/op_graph/lib/linux/x86_64
    cp build/libcust_opapi.so output/op_graph/lib/linux/x86_64/

    # 配置环境变量并验证
    export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"
    ls "$ASCEND_CUSTOM_OPP_PATH"/op_graph/lib/linux/x86_64/   # 应看到libcust_opapi.so
    ```

4. 运行验证

    ```python
    python3 script/add_custom_test.py
    ```

    若终端窗口出现 Ran 1 test和OK，则表示eager模式和TorchAir图模式均验证通过。

## 样例解析

1. **阶段1：算子定义**

    算子定义阶段声明算子的输入、输出、属性等元信息。本样例中，GE侧REG\_OP定义和PyTorch侧TORCH\_LIBRARY注册分别在两个C++文件中完成。

    1. **GE侧REG\_OP（custom\_op.cpp）：**

        ```c++
        // add_custom_kernel/custom_op.cpp
        // 注册GE算子原型：声明AddCustom算子的输入、输出及支持的数据类型
        REG_OP(AddCustom)
            .INPUT(x, "T")    // 输入x
            .INPUT(y, "T")    // 输入y
            .OUTPUT(z, "T")   // 输出z
            .DATATYPE(T, TensorType({DT_BOOL, DT_FLOAT, DT_INT32, DT_INT64, DT_FLOAT16, DT_BF16,
                                     DT_INT16, DT_INT8, DT_UINT8, DT_DOUBLE, DT_COMPLEX128,
                                     DT_COMPLEX64, DT_STRING, DT_COMPLEX32}))
            .OP_END_FACTORY_REG(AddCustom);
        ```

        同时在该文件中实现InferShape推导函数并注册：

        ```c++
        // add_custom_kernel/custom_op.cpp
        // AddCustom算子的形状推导函数，实现PyTorch风格的广播语义，GE在图编译期调用此函数，根据输入shape推导输出shape
        graphStatus InferShapeForAdd(gert::InferShapeContext *context) {
          // 获取两个输入的shape
          auto input_shape_0 = *context->GetInputShape(0);
          auto input_shape_1 = *context->GetInputShape(1);
          // 获取输出shape的可写引用，用于设置推导结果
          auto output_shape = context->GetOutputShape(0);
          // 情况1：两个输入维度数不同，处理广播语义（一维张量广播到多维）
          if (input_shape_0.GetDimNum() != input_shape_1.GetDimNum()) {
            auto min_num = std::min(input_shape_0.GetDimNum(), input_shape_1.GetDimNum());
            auto max_num = std::max(input_shape_0.GetDimNum(), input_shape_1.GetDimNum());
            if (min_num != 1) {
              std::cerr << "Add param invalid" << std::endl;
            } else {
              if (input_shape_1.GetDimNum() > 1) {
                *output_shape = input_shape_1;
              } else {
                *output_shape = input_shape_0;
              }
              return GRAPH_SUCCESS;
            }
          }
          // 情况2：某个输入为标量（维度数为0），输出shape取另一个输入的shape
          if (input_shape_0.GetDimNum() == 0) {
            *output_shape = input_shape_1;
            return GRAPH_SUCCESS;
          }
          // 情况3：两个输入维度数相同，逐维取最大值（标准广播规则）
          if (input_shape_1.GetDimNum() == 0) {
            *output_shape = input_shape_0;
            return GRAPH_SUCCESS;
          }
          output_shape->SetDimNum(input_shape_0.GetDimNum());
          for (size_t i = 0; i < input_shape_0.GetDimNum(); ++i) {
            output_shape->SetDim(i, std::max(input_shape_0.GetDim(i), input_shape_1.GetDim(i)));
          }
          return GRAPH_SUCCESS;
        }

        // 将InferShape推导函数注册到AddCustom算子，GE在编译期遇到AddCustom节点时，会调用此函数推导输出shape
        IMPL_OP(AddCustom).InferShape(InferShapeForAdd);
        ```

    2. **PyTorch侧TORCH\_LIBRARY注册（add\_custom.asc）：**

        在.asc文件中，通过TORCH\_LIBRARY\_FRAGMENT定义算子schema，通过TORCH\_LIBRARY\_IMPL绑定各DispatchKey的实现：

        ```c++
        // add_custom_kernel/add_custom.asc
        // PyTorch侧算子实现：在NPU上执行Ascend C kernel
        namespace ascendc_ops {
        // ascendc_add函数：eager模式下被PyTorch调用
        // 负责准备参数并启动Ascend C核函数
        at::Tensor ascendc_add(const at::Tensor& x, const at::Tensor& y) {
          // 获取当前NPU执行流，kernel在此stream上异步执行
          auto aclStream = c10_npu::getCurrentNPUStream().stream(false);
          // 分配输出Tensor，shape与输入x相同
          at::Tensor z = at::empty_like(x);
          // 设置核函数启动的block数量
          uint32_t numBlocks = 8;
          // 计算输入元素总数（所有维度相乘）
          uint32_t totalLength = 1;
          for (uint32_t size : x.sizes()) {
            totalLength *= size;
          }
          // 调用封装函数启动核函数
          launch_add_custom(
            static_cast<uint8_t*>(x.data_ptr()),  // 输入x的设备地址
            static_cast<uint8_t*>(y.data_ptr()),  // 输入y的设备地址
            static_cast<uint8_t*>(z.data_ptr()),  // 输出z的设备地址
            totalLength,                          // 元素总数
            numBlocks,                            // 核函数block数
            aclStream                             // 执行流
          );
          return z;
        }
        } // namespace ascendc_ops

        // ===== PyTorch算子注册部分 =====
        // 定义算子schema：声明函数签名，供PyTorch识别
        // ascendc_ops为命名空间，ascendc_add为函数名
        TORCH_LIBRARY_FRAGMENT(ascendc_ops, m) {
          m.def("ascendc_add(Tensor x, Tensor y) -> Tensor");
        }

        // 注册PrivateUse1实现：NPU设备上的eager执行路径
        // 当用户在NPU上直接调用torch.ops.ascendc_ops.ascendc_add(x, y)时触发
        TORCH_LIBRARY_IMPL(ascendc_ops, PrivateUse1, m) {
          m.impl("ascendc_add", TORCH_FN(ascendc_ops::ascendc_add));
        }

        // 注册XLA实现：图编译路径
        // 当算子通过torch.compile进入图模式时，XLA路径会触发GE图编译
        TORCH_LIBRARY_IMPL(ascendc_ops, XLA, m) {
          m.impl("ascendc_add", TORCH_FN(ascendc_ops::ascendc_add));
        }

        // 注册Meta实现：编译期shape推导
        // Meta路径不执行实际计算，仅返回与输入同shape的空Tensor
        // TorchAir在图编译阶段调用此实现进行shape推导
        TORCH_LIBRARY_IMPL(ascendc_ops, Meta, m) {
          m.impl("ascendc_add", [](const torch::Tensor& x, const torch::Tensor& y) {
            return at::empty_like(x);
          });
        }
        ```

        **说明**：本样例将PyTorch注册逻辑放在.asc文件中（C++源码），与kernel实现同库编译，是Ascend C算子入图的常见做法，kernel与注册逻辑统一在一个编译单元中完成。

2. **阶段2：前端接入**

    前端接入通过TorchAir converter将PyTorch FX节点映射到GE自定义算子。本样例中converter注册在测试脚本中完成：

    ```python
    # script/add_custom_test.py
    import torchair
    from torchair import register_fx_node_ge_converter
    from torchair.ge import Tensor, TensorSpec

    # 注册GE converter：将PyTorch FX图中的自定义算子节点映射为GE图中的算子节点
    # 装饰器指定要转换的PyTorch算子引用（torch.ops.ascendc_ops.ascendc_add.default）
    # TorchAir在图编译阶段遇到此算子时，会调用此converter函数
    @torchair.register_fx_node_ge_converter(torch.ops.ascendc_ops.ascendc_add.default)
    def convert_ascendc_ascendc_add(
        x: Tensor, y: Tensor, z: Tensor = None, meta_outputs: TensorSpec = None
    ):
        # 构造GE自定义算子节点并返回
        return torchair.ge.custom_op(
            "AddCustom",            # GE算子类型名，必须与REG_OP和REG_AUTO_MAPPING_OP一致
            inputs={
                "x": x,             # 将PyTorch输入x映射到GE算子的输入端口x
                "y": y,             # 将PyTorch输入y映射到GE算子的输入端口y
            },
            outputs=["z"],          # 声明GE算子输出端口名，与REG_OP中.OUTPUT(z, ...)对应
        )
    ```

    > [!NOTE]说明
    > converter中的"AddCustom"必须与REG\_OP和REG\_AUTO\_MAPPING\_OP中的op type一致。

3. **阶段4-5：GE交付件实现**

    GE交付件在custom\_op.cpp中实现EagerExecuteOp::Execute，完成kernel启动：

    ```c++
    // add_custom_kernel/custom_op.cpp
    // AddCustom算子的GE交付件实现类，继承EagerExecuteOp，实现Execute方法完成kernel启动
    class AddCustom : public EagerExecuteOp {
     public:
      // Execute回调：GE在图执行阶段调用此方法，负责启动Ascend C核函数
      graphStatus Execute(gert::EagerOpExecutionContext *ctx) {
        // 获取算子输入Tensor（包含设备地址、shape、dtype、format等元信息）
        const gert::Tensor *input_x = ctx->GetInputTensor(0);
        const gert::Tensor *input_y = ctx->GetInputTensor(1);
        if (input_x == nullptr || input_y == nullptr) {
          std::cerr << "Input tensor is null!" << std::endl;
          return GRAPH_FAILED;
        }
        // 2. 从输入Tensor中提取元信息，用于分配输出Tensor
        const gert::StorageShape &output_shape = input_x->GetShape();
        uint32_t tensor_size = input_x->GetSize();
        DataType data_type = input_x->GetDataType();
        const gert::StorageFormat &format = input_x->GetFormat();

        // 3. 申请输出Tensor的设备内存（由GE context管理生命周期，无需手动释放）
        gert::Tensor *output_z = ctx->MallocOutputTensor(0, output_shape, format, data_type);
        if (output_z == nullptr) {
          std::cerr << "Failed to malloc output tensor memory!" << std::endl;
          return GRAPH_FAILED;
        }
        // 4. 计算核函数启动参数：grid维度和block数量
        int64_t n_elements = input_x->GetShapeSize();
        const int32_t BLOCK_SIZE_VALUE = 1024;
        int32_t grid_x = std::ceil(static_cast<double>(n_elements) / (BLOCK_SIZE_VALUE));
        int32_t grid_y = 1;
        int32_t grid_z = 1;
        uint32_t num_block = grid_x * grid_y * grid_z;

        // 5. 获取输入输出的设备地址
        void *z_addr = output_z->GetAddr();
        void *x_addr = const_cast<void *>(input_x->GetAddr());
        void *y_addr = const_cast<void *>(input_y->GetAddr());
        // 6. 获取执行流并启动核函数（异步执行，立即返回）
        aclrtStream stream = ctx->GetStream();
        // 调用封装函数启动Ascend C核函数add_custom
        launch_add_custom(static_cast<uint8_t *>(x_addr), static_cast<uint8_t *>(y_addr),
                          static_cast<uint8_t *>(z_addr), tensor_size, num_block, stream);
        return GRAPH_SUCCESS;
      }
    };

    // 将AddCustom实现类注册到GE
    // GE在图中遇到"AddCustom"类型节点时，会实例化此类并回调Execute方法
    REG_AUTO_MAPPING_OP(AddCustom);
    ```

    其中launch\_add\_custom在add\_custom\_kernel.h中声明，在add\_custom.asc中实现，负责调用Ascend C核函数add\_custom。

## 调试与错误排查

上述开发过程中如果存在GE找不到自定义算子或ASCEND\_CUSTOM\_OPP\_PATH未配置或.so不在op\_graph/lib/<os\>/<arch\>/路径，请参见[调试与常见问题](debug_and_common_issues.md)。
