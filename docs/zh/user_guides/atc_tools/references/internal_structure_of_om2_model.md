# om2模型内部结构

在Linux服务器上执行**unzip  _\*.om2_**命令解压模型文件。进入解压后的目录，其内部结构如下所示：

```tree
<model_name>
├── manifest.json                                   # 元信息：CANN版本、序列化协议（JSON文件中的内容）、生成方式等
└── data
    │── model_{model_no}                            # {model_no}：表示打包到om2中的模型序号，只有一个模型则序号为0
    │    ├─ debug                                   # debug信息
    │    │   ├─ ge_visual_xxxx_graph_0.json         # 该图信息支持可视化，可使用Netron等工具打开查看
    │    │   └─ op_attr.json                        # 算子的属性信息
    │    ├─ model_meta.json                         # 模型的基本信息
    │    └─ runtime                                 # 模型执行时
    │        ├─ csrc                                # 模型源码和工程文件
    │        │   ├─ <model_name>_args_manager.cpp   # 模型中算子的args管理模块
    │        │   ├─ <model_name>_internal.h         # 头文件，OM2模型内部使用
    │        │   ├─ <model_name>_kernel_reg.cpp     # 模型中算子二进制kernel注册的实现
    │        │   ├─ <model_name>_load_and_run.cpp   # 模型中算子加载及执行的接口实现
    │        │   ├─ <model_name>_resources.cpp      # 模型资源的初始化实现
    │        │   ├─ Makefile                        # 编译工程文件
    │        │   └─ om2_model_api.h                 # 头文件，包含模型加载、执行、析构的接口声明，涉及与 OM2 executor 交互
    │        └─ lib<model_name>_om2.so              # 模型执行时代码编译生成的so
    ├── kernels                                     # 算子二进制
    │    └─ te_add_xxxx.o
    ├── custom_ops                                  # 自定义算子
    │    ├─ binaries_npu_arch                       # 自定义算子二进制，文件名格式： kernelName_hashId_CustomKernel.bin
    │    │   └─ <kernel_name>_<file_hash>_CustomKernel.bin
    │    └─ shared_libs                             # 自定义算子 host so
    │        └─ <file_size>_<file_hash>_<original_so_name>.so
    └── constants                                   # 所有模型的常量与权重数据
          ├─ model_{model_no}_constants_config.json # model_{model_no}的常量与权重数据
          └─ constant_{model_no}                    # 内置权重
```

内部结构中的JSON文件及包括的字段详细说明请分别参见[manifest.json](#manifestjson)、[model\_meta.json](#model_metajson)和[model\_\{model\_no\}\_constants\_config.json](#model_model_no_constants_configjson)，文件中的数据均为示例，请以实际模型中的信息为准。

## manifest.json

```json
{
    "atc_command": "/usr/local/Ascend/cann-xxx/{arch}-linux/bin/atc.bin --model=./resnet50.onnx --framework=5 --output=./resnet50 --soc_version={soc_version} --mode=7 --external_weight=1",
    "model_num": 1,
    "om2_version": "0"
}
```

- atc\_command：生成om2的atc命令。
- model\_num：包含的模型数量。
- om2\_version：om2版本。

## model\_meta.json

```json
{
    "dynamic_batch_info": [],
    "dynamic_output_shape": [],
    "dynamic_type": 0,
    "inputs": [
        {
            "data_type": "DT_FLOAT",
            "format": "NCHW",
            "index": 0,
            "name": "actual_input_1",
             "origin_input_dims": [
                16,
                3,
                224,
                224
            ],
            "shape": [
                16,
                3,
                224,
                224
            ],
            "shape_range": [],
            "shape_v2": [
                16,
                3,
                224,
                224
            ],
            "size": 9633792
        }
    ],
    "name": "resnet50",
    "outputs": [
        {
            "data_type": "DT_FLOAT",
            "format": "ND",
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
    "root_graph_name": "tc_ge_acl_support_om2_0001",
    "user_designate_shape_order": [],
    "work_size": 86770688,
    "zero_copy_size": 2255360
}
```

- dynamic\_batch\_info：用于生成模型描述信息中的输入所支持的动态batch信息。
- dynamic\_output\_shape：用于生成模型描述信息中的dynamicOutputShape字段。
- dynamic\_type：用于生成模型描述信息中的输入所支持的动态维度信息。
- inputs：模型输入信息，为一个数组，详细字段如下：
    - data\_type：输入的数据类型。
    - format：输入的format。
    - index：输入的索引编号。
    - name：输入名称。
    - origin\_input\_dims: 输入tensor的原始维度信息。
    - shape：输入tensor的维度信息，动态分档场景取值为最大档位对应的维度信息。
    - shape\_range：输入tensor的维度范围。
    - shape\_v2：通过接口获取的维度信息，H维度的值=输入图像的H维度值\*系数，不同图片格式下C维度的值不同，详细规则请参见该接口处的表1所示，且各维度值相乘的结果值与通过接口获取的值保持一致。
    - size：输入tensor大小。

- name：模型名称。
- outputs：模型输出信息，为一个数组，详细字段如下：
    - data\_type：输出的数据类型。
    - format：输出的format。
    - index：输出的索引编号。
    - name：输出名称。
    - shape：输出tensor的维度信息。
    - shape\_range：输出tensor的维度范围。
    - shape\_v2：通过接口获取的维度信息，H维度的值 = 输入图像的H维度值\*系数，不同图片格式下C维度的值不同，详细规则请参见该接口处的表1所示，且各维度值相乘的结果值与通过接口获取的值保持一致。
    - size：输出tensor大小。

- root\_graph\_name：根图名称。
- user\_designate\_shape\_order：内部使用字段。用户无需关注。
- work\_size：模型使用到的工作内存大小。单位为字节。
- zero\_copy\_size：零拷贝大小，单位为字节。

## model\_\{model\_no\}\_constants\_config.json

```json
{
    "consts": {
        "dynamic_const_523670_107_78daf6e19d54b25d13695263c5bf9533ef419ab3cecc6c863d31e58125ee2e": {
            "file_name": "weight_78daf6e19d54b25d13695263c5bf9533ef419ab3cecc6c863d31e58125ee2e",
            "index": 107,
            "offset": 0,
            "op_name": "dynamic_const_523670_107_78daf6e19d54b25d13695263c5bf9533ef419ab3cecc6c863d31e58125ee2e",
            "size": 4096000,
            "type": "INDIVIDUAL"
        },
        "dynamic_const_523670_108_f2c712e3e81fb5fd0a67fa0edeba4e7e1943a4c04e7fff1679708ece3820ef": {
            "file_name": "weight_f2c712e3e81fb5fd0a67fa0edeba4e7e1943a4c04e7fff1679708ece3820ef",
            "index": 106,
            "offset": 0,
            "op_name": "dynamic_const_523670_108_f2c712e3e81fb5fd0a67fa0edeba4e7e1943a4c04e7fff1679708ece3820ef",
            "size": 2000,
            "type": "COMBINED"
        },
        "dynamic_const_523670_110_9182a618fc53916194dfd9963d83999dacf0887d50e635805f34adf627eb6": {
            "file_name": "",
            "index": 105,
            "offset": 0,
            "op_name": "dynamic_const_523670_110_9182a618fc53916194dfd9963d83999dacf0887d50e635805f34adf627eb6",
            "size": 128,
            "type": "INTERNAL"
        }
    },
    "internal_weight_size": 128
}
```

- consts：模型中的const节点数组。
    - key：如果有op\_name则显示op\_name，否则显示constant\_\{index\}。
        - file\_name：外置权重const节点对应的权重文件名。
        - index：const节点对应的索引，内部使用。
        - offset：const节点对应的权重内存偏移，内部使用。
        - op\_name：对应的计算图节点名。
        - size：权重大小。
        - type：表示权重类型，包括如下取值：
            - INTERNAL：内部权重。
            - INDIVIDUAL：权重外置但不归一。
            - COMBINED：权重外置且归一。

- internal\_weight\_size：内置权重的总大小。如果权重全部外置，则该值取值为0。
