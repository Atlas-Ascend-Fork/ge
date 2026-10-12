# 交付件与OPP规范

## 规范目录树（唯一真值）

自定义算子以.so 交付件形式被GE加载，通过ASCEND\_CUSTOM\_OPP\_PATH环境变量指定OPP包根目录。以下为全文统一的目录结构规范：

```tree
<custom_opp_root>/
├── op_graph/                          # GE侧交付件（核心）
│   ├── include/                       # REG_OP proto头文件（供构图侧/Parser使用）
│   │   └── my_custom_op.h
│   └── lib/
│       └── <os>/<arch>/               # 如linux/x86_64、linux/aarch64
│           └── libcust_opapi.so       # GE交付件（统一命名）
├── framework/                         # 框架侧交付件（按需）
│   ├── tensorflow/
│   │   ├── libcustom_ops.so           # TF侧算子声明（TF场景）
│   │   └── npu_supported_ops.json     # Adapter配置（TF场景）
│   └── onnx/
│       └── libonnx_parser_plugin.so   # ONNX解析插件（ONNX 场景）
└── python/                            # PyTorch侧交付件（按需，不参与OPP加载）
    ├── add_custom.py                  # TORCH_LIBRARY定义
    └── add_custom_converter.py        # converter注册
```

## 目录项说明

| 目录 | 内容 | 何时需要 |
| --- | --- | --- |
| op_graph/include/ | REG_OP proto头文件 | GE原生/PyTorch/ONNX 场景（供构图侧或Parser创建节点） |
| op_graph/lib/<os\>/<arch\>/ | GE交付件.so（回调实现） | 所有场景 |
| framework/tensorflow/ | TF侧声明+Adapter 配置 | 仅TensorFlow场景 |
| framework/onnx/ | ONNX解析插件.so | 仅ONNX场景 |
| python/ | PyTorch schema+converter | 仅PyTorch场景；该目录不参与OPP加载，由Python侧按需import |

**kernel binary位置**：预编译kernel binary不强制放入OPP包内，可与.so一同编译，或由Execute阶段从文件加载（需确保路径符合OPP包结构约定，且**不依赖临时工作目录**，具体请参见[调试与常见问题](debug_and_common_issues.md)）。

## 命名约定

| 项 | 约定 | 说明 |
| --- | --- | --- |
| GE交付件 | libcust_opapi.so | 统一命名，便于识别与脚本处理 |
| 解析插件 | libonnx_parser_plugin.so | ONNX场景 |
| TF侧算子 | libcustom_ops.so | TensorFlow场景 |
| REG_OP头文件 | op_graph/include/\<op>.h | 与op type同名便于检索 |
| \<os>/\<arch> | 如linux/x86_64 | 按目标平台填写，缺一不可 |

## 环境变量与加载机制

```bash
export ASCEND_CUSTOM_OPP_PATH="<custom_opp_root>:$ASCEND_CUSTOM_OPP_PATH"
```

- 支持叠加系统自带路径：**先写自定义路径，再追加原值**（:$ASCEND\_CUSTOM\_OPP\_PATH）。
- GE在$ASCEND\_CUSTOM\_OPP\_PATH/op\_graph/lib/<os\>/<arch\>/下查找交付件.so。
- ONNX解析插件与GE交付件需**同时加载**，且解析插件必须在ATC编译前完成加载。
- PyTorch的`python/`目录不通过该变量加载，按Python包方式安装`/`引用即可。

## 部署步骤

以编译产物在`build/`下为例：

```bash
# 1. 组织OPP结构
mkdir -p output/op_graph/lib/linux/x86_64
cp build/libcust_opapi.so output/op_graph/lib/linux/x86_64/

# 2. 若使用CMake install规则，可替代上一步
cmake --install build --prefix "$(pwd)/output"

# 3. 配置环境变量
export ASCEND_CUSTOM_OPP_PATH="$(pwd)/output:$ASCEND_CUSTOM_OPP_PATH"

# 4. 验证GE可找到交付件
ls "$ASCEND_CUSTOM_OPP_PATH"/op_graph/lib/linux/x86_64/
```

完整端到端示例见[快速开始](quick_start.md)。
