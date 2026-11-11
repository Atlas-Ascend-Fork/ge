# om2模型内部结构

在Linux服务器上执行unzip  \*.om2命令解压模型文件。进入解压后的目录，其内部结构如下所示：

```tree
<model_name>
├── manifest.json                                   # 包清单：版本兼容性信息、模型数量、ATC命令等
└── data
    ├── model_{model_no}                            # {model_no}：表示打包到om2中的模型序号，只有一个模型则序号为0
    │   ├── debug                                  # debug信息
    │   │   └── ge_visual_00000000_graph_0.json     # 该图信息支持可视化，可使用Netron等工具打开查看
    │   ├── model_meta.json                        # 模型的基本信息
    │   ├── op_attr.json                           # 算子的属性信息
    │   ├── constants_config.json                  # 模型的常量与权重配置
    │   ├── variables_config.json                  # 变量配置
    │   └── runtime                                # 模型执行时
    │       ├── csrc                               # 模型源码和工程文件
    │       │   ├── <model_name>_args_manager.cpp   # 模型中算子的args管理模块
    │       │   ├── <model_name>_internal.h         # 头文件，OM2模型内部使用
    │       │   ├── <model_name>_kernel_reg.cpp     # 模型中算子二进制kernel注册的实现
    │       │   ├── <model_name>_load_and_run.cpp   # 模型中算子加载及执行的接口实现
    │       │   ├── <model_name>_resources.cpp      # 模型资源的初始化实现
    │       │   ├── Makefile                       # 编译工程文件
    │       │   └── om2_model_api.h                # 头文件，包含模型加载、执行、析构的接口声明，涉及与 OM2 executor 交互
    │       └── lib<model_name>_om2.so              # 模型执行时代码编译生成的so
    ├── kernels                                    # 算子二进制
    │   └── te_add_xxxx.o
    ├── custom_ops                                 # 自定义算子
    │   ├── binaries_npu_arch                      # 自定义算子二进制，文件名格式： kernelName_hashId_CustomKernel.bin
    │   │   └── <kernel_name>_<file_hash>_CustomKernel.bin
    │   └── shared_libs                            # 自定义算子 host so
    │       └── <file_size>_<file_hash>_<original_so_name>.so
    ├── constants                                  # 所有模型的常量与权重数据
    │   └── constant_{model_no}                     # 内置权重
    └── variables                                  # 变量初始化数据
        └── var_weight_data_{model_no}              # 对应模型的变量初始化数据
```

内部结构中的JSON文件及包括的字段详细说明请分别参见[manifest.json](#manifestjson)、[model\_meta.json](#model_metajson)和[constants\_config.json](#constants_configjson)，文件中的数据均为示例，请以实际模型中的信息为准。

## manifest.json

```json
{
    "atc_command": "/usr/local/Ascend/cann-xxx/{arch}-linux/bin/atc.bin --model=./resnet50.onnx --framework=5 --output=./resnet50 --soc_version={soc_version} --mode=7 --external_weight=1",
    "compatibility": {
        "compiler_version": "1.0",
        "required_executor_version": "",
        "used_features": {}
    },
    "model_num": 1
}
```

- atc\_command：生成om2的atc命令。
- model\_num：包含的模型数量。
- compatibility：版本兼容性信息。
    - compiler\_version：生成模型的编译器版本。
    - required\_executor\_version：模型所需的最低执行器版本，取值为本模型使用的各特性要求的执行器版本中的最大值；未使用任何特性时为空字符串。
    - used\_features：模型使用的特性名称到特性所需执行器版本字符串的映射；未使用特性时为空对象。

以上版本号仅为示例，请以实际模型中的信息为准。

## model\_meta.json

```json
{
    "dynamic_dims": {
        "dynamic_type": 1,
        "user_designate_shape_order": ["actual_input_1"],
        "gears": [
            {
                "inputs": [1],
                "outputs": [[1, 1000]]
            },
            {
                "inputs": [16],
                "outputs": [[16, 1000]]
            }
        ]
    },
    "inputs": [
        {
            "data_type": 0,
            "format": 0,
            "index": 0,
            "name": "actual_input_1",
            "shape": [
                -1,
                3,
                224,
                224
            ],
            "max_gear_shape": [
                16,
                3,
                224,
                224
            ],
            "shape_range": [],
            "size": 9633792
        }
    ],
    "name": "resnet50",
    "outputs": [
        {
            "data_type": 0,
            "format": 2,
            "index": 0,
            "name": "PartitionedCall_Gemm_174_add_4:0:output1",
            "shape": [
                16,
                1000
            ],
            "shape_range": [],
            "size": 64000
        }
    ],
    "work_size": 86770688,
    "zero_copy_size": 2255360
}
```

- inputs：模型输入信息，为一个数组，详细字段如下：
    - data\_type：输入的数据类型。
    - format：输入的format。
    - index：输入的索引编号。
    - name：输入名称。
    - shape：输入tensor的维度信息，动态分档场景取值为原始输入的维度信息。
    - shape\_range：输入tensor的维度范围。
    - max\_gear\_shape：最大档位对应的输入维度，仅在动态分档场景生成。
    - shape\_aclmdlGetInputDimsV2：用于`aclmdlGetInputDimsV2`查询的输入维度，仅在模型的AIPP配置列表非空时生成。
    - size：输入tensor大小。

- name：模型名称。
- outputs：模型输出信息，为一个数组，详细字段如下：
    - data\_type：输出的数据类型。
    - format：输出的format。
    - index：输出的索引编号。
    - name：输出名称。
    - shape：输出tensor的维度信息。
    - shape\_range：输出tensor的维度范围。
    - size：输出tensor大小。

- work\_size：模型使用到的工作内存大小。单位为字节。
- zero\_copy\_size：零拷贝大小，单位为字节。
- dynamic\_dims：动态分档配置，仅在模型的动态档位信息非空时生成，详细字段如下：
    - dynamic\_type：用于生成模型描述信息中的输入所支持的动态维度信息。
    - user\_designate\_shape\_order：用户指定的输入shape信息顺序，为字符串数组。
    - gears：档位数组，每项包含inputs和outputs。
        - inputs：当前档位各动态维度的取值，为整数数组。
        - outputs：当前档位各输出的维度，为二维整数数组。
- aipp：AIPP配置，仅在模型的AIPP配置列表非空时生成。其中aipp\_infos为配置数组，仅保留非空且类型不为`DATA_WITHOUT_AIPP`的条目。

## constants\_config.json

```json
{
    "consts": {
        "dynamic_const_523670_107_78daf6e19d54b25d13695263c5bf9533ef419ab3cecc6c863d31e58125ee2e": {
            "file_name": "weight_78daf6e19d54b25d13695263c5bf9533ef419ab3cecc6c863d31e58125ee2e",
            "index": 107,
            "offset": 0,
            "size": 4096000,
            "type": "INDIVIDUAL"
        },
        "dynamic_const_523670_108_f2c712e3e81fb5fd0a67fa0edeba4e7e1943a4c04e7fff1679708ece3820ef": {
            "file_name": "weight_f2c712e3e81fb5fd0a67fa0edeba4e7e1943a4c04e7fff1679708ece3820ef",
            "index": 106,
            "offset": 0,
            "size": 2000,
            "type": "INDIVIDUAL"
        },
        "constant_105": {
            "file_name": "constant_0",
            "index": 105,
            "offset": 0,
            "size": 128,
            "type": "INTERNAL"
        }
    },
    "internal_weight_size": 128
}
```

- consts：模型中的const节点配置对象。
    - key：内部权重使用constant\_\{index\}，外置权重使用对应的计算图节点名。
        - file\_name：const节点对应的权重文件名，内部权重为constant\_\{model\_no\}。
        - index：const节点对应的索引，内部使用。
        - offset：const节点对应的权重文件偏移，单位为字节，内部使用。
        - size：权重大小。
        - type：表示权重类型，包括如下取值：
            - INTERNAL：内部权重。
            - INDIVIDUAL：权重外置但不归一。
            - COMBINED：权重外置且归一。

- internal\_weight\_size：内置权重的总大小。如果权重全部外置，则该值取值为0。
