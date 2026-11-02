# 样例使用指导<a name="ZH-CN_TOPIC_0345664697"></a>

## 功能描述<a name="section5991635456363"></a>

本样例为 Conv2D/Conv2DV2/ExtendConv2D 与 Concat 零拷贝标记的自定义 pass 样例，识别 Conv2D/Conv2DV2/ExtendConv2D+Concat 结构后，通过给节点设置属性的方式将 Concat 标记为虚拟算子（不生成 Task、输出复用输入内存），并描述各 conv 输出在 Concat 输出连续内存中的排布信息。pass 通过 GetAllNodes 遍历图内所有节点（含子图节点）。

识别的固定 pattern：

```
   a         b
    \       /
    Conv2D  ExtendConv2D  ==>   conv0_out + conv1_out 按序连续排布
       \      /                  Concat 标记为虚拟算子：不生成Task、输出复用输入内存
       ConcatD
```

设置的属性说明：

| 节点 | 属性名 | 类型 | 说明 |
|------|--------|------|------|
| ConcatD/ConcatV2D | `_no_task`（ATTR_NAME_NOTASK） | bool | 标记该算子不生成硬件 Task |
| ConcatD/ConcatV2D | `_output_reuse_input`（ATTR_NAME_OUTPUT_REUSE_INPUT） | bool | 标记输出复用输入内存 |
| ConcatD/ConcatV2D | `_no_padding_continuous_input`（ATTR_NAME_NOPADDING_CONTINUOUS_INPUT） | bool | 标记输入为无 padding 的连续内存 |
| ConcatD/ConcatV2D | `_reuse_input_on_dim_index`（ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX） | int64 | 输出复用输入内存的维度索引（本例为比较输入输出 shape 求出的真实 concat 轴） |
| Conv2D/Conv2DV2/ExtendConv2D | `_output_offset_list_for_continuous`（ATTR_NAME_OUTPUT_OFFSET_LIST_FOR_CONTINUOUS） | list\<int64\> | 各输出在 concat 输出连续内存中相对于第 1 个 conv 输出地址的字节偏移 |
| Conv2D/Conv2DV2/ExtendConv2D | `total_shape`（私有属性） | list\<int64\> | conv 输出所在 concat 输出的完整 shape |
| Conv2D/Conv2DV2/ExtendConv2D | `valid_shape`（私有属性） | list\<int64\> | conv 自身输出的有效 shape |

匹配条件：

- ConcatD/ConcatV2D 的所有输入均来自不同的 Conv2D/Conv2DV2/ExtendConv2D 节点，且对应 conv 输出仅被该 Concat 消费；
- 每个 conv 输出 format 为 NCHW 或 NHWC，且 C 维度 32 字节对齐（C 维度大小 = C 轴大小 × dtype 大小，为 32B 的整数倍；NCHW 中 C 轴为 dim 1，NHWC 中 C 轴为最内层 dim 3），保证各输入在 concat 连续内存中的起始地址 32B 对齐（强于 GE 内置 `CheckPhonyConcatInputIs32Align` 的张量大小 32B 对齐约束）；
- concat 的拼接轴（concat_dim）与 conv 输出的 C 轴对应，支持负索引（从最后一维起算）：NCHW 为 dim 1 或 -3，NHWC 为 dim 3 或 -1；
- 输入输出 shape 为静态 shape，数据类型为常见定长类型。

> 说明：本样例仅演示属性设置方式，未覆盖 GE 内置 Concat no-task 优化的完整校验链（如 32 字节对齐、前驱节点类型、级联 continuous input 等）。实际使用时需结合内存校验确保各 conv 输出地址满足连续性要求，否则可能导致运行结果错误。

## 目录结构<a name="section7668345634665"></a>

```
├── src
│   ├──conv_concat_notask_pass.cpp  // pass实现文件
├── CMakeLists.txt                  // 编译脚本
├── data
│   ├── gen_onnx.py                 // ONNX 模型生成脚本（Conv2D+Concat 结构）
│   └── run_atc.sh                  // ATC 转换脚本（带 DUMP_GE_GRAPH）
```

## 环境要求<a name="section383335652346"></a>

- 编译器：GCC >= 7.3.x
- 使用python及其依赖库版本：python>=3.9、onnx
- 已完成[相关环境准备](../../../../../docs/zh/build.md)。

## 程序编译<a name="section6645633456813"></a>

假设CANN软件包的安装目录为INSTALL_PATH，例如`/home/HwHiAiUser/Ascend/`。

1. 配置环境变量。

   运行软件包中设置环境变量脚本，命令如下：

   ```
   source ${ASCEND_PATH}/set_env.sh
   ```

   `${ASCEND_PATH}`为CANN软件包安装目录下的cann路径。请替换相关软件包的实际安装路径，例如`${INSTALL_PATH}/cann`。

2. 根据实际情况修改**CMakeLists.txt**文件中的如下信息。

    - ASCEND_PATH：可以设置默认的软件包路径，如果通过set_env.sh设置了`$ASCEND_HOME_PATH`，无需修改。

    - FUSION_PASS_DIR：可以设置自定义融合pass动态库安装目录名，默认为`fusion_passes`。

3. 执行如下命令进行编译，编译结束后，在**build**目录下生成动态库文件**libconv_concat_notask_pass.so**。

   依次执行:

   ```
   mkdir build && cd build
   cmake .. && make
   ```

4. 成功编译后通过make install将动态库文件libconv_concat_notask_pass.so安装到自定义融合pass目录下。

   ```
   make install
   ```

   样例验证完成后，执行如下命令清理安装到 CANN 包下的自定义 pass so，避免影响后续 UT/ST：

   ```
   make clean_custom_pass
   ```

## 程序运行<a name="section4524573456563512"></a>

1. 配置环境变量(如已执行，跳过)。

   - 运行软件包中设置环境变量脚本，命令如下：

     ```
     source ${ASCEND_PATH}/set_env.sh
     ```

     `${ASCEND_PATH}`请替换相关软件包的实际安装路径。

2. 使用ATC离线推理。

    - 在**data**目录执行模型生成脚本，生成包含 Conv2D+Concat 结构的 ONNX 模型：

      **python3 gen_onnx.py**

      执行结束后，在**data**目录下生成**model.onnx**模型文件。

    - 执行ATC命令，其中soc_version根据实际模型运行环境填写（本pass仅在mc62版本芯片上生效，即soc_version以MC62开头）：

      **atc --model=./model.onnx --framework=5 --soc_version=xxx --output=./model**

      或直接使用**data**目录下的基础运行脚本（脚本内部已设置DUMP_GE_GRAPH等环境变量并执行ATC转换）：

      ```
      bash run_atc.sh MC62
      ```

    - 检查执行结果：

      - 日志中出现如下打印：

        ```
        ConvConcatNotaskPass begin.
        ConvConcatNotaskPass end.
        ```

      - 本pass在入口处校验芯片版本，仅mc62版本芯片执行后续标记流程；其他芯片pass直接跳过，日志中不会出现属性标记相关内容，仅打印如下：

        ```
        ConvConcatNotaskPass begin.
        Soc version is not MC62, skip.
        ConvConcatNotaskPass end.
        ```

      - 对比NPU编译过程中间dump图（获取方法请参见CANN环境变量说明>编译相关>DUMP_GE_GRAPH），Concat节点上已携带`_no_task`等属性，各Conv2D节点上已携带`_output_offset_list_for_continuous`等属性。
