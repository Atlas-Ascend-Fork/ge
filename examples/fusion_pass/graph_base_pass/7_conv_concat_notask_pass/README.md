# 样例使用指导<a name="ZH-CN_TOPIC_0345664697"></a>

## 功能描述<a name="section5991635456363"></a>

本样例为 Conv2D/Conv2DV2/ExtendConv2D 与 Concat 零拷贝标记的自定义 pass 样例，识别 Conv2D/Conv2DV2/ExtendConv2D+Concat 结构后，通过给节点设置属性的方式将 Concat 标记为虚拟算子（不生成 Task、输出复用输入内存），并描述各 conv 输出在 Concat 输出连续内存中的排布信息。

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
- 每个 conv 输出张量字节数（C 轴及后续维度连续排布的大小）为 32B 的整数倍，保证各输入在 concat 连续内存中的起始地址 32B 对齐（与 GE 内置 `CheckPhonyConcatInputIs32Align` 约束一致）；
- 输入输出 shape 为静态 shape，数据类型为常见定长类型。

> 说明：本样例仅演示属性设置方式，未覆盖 GE 内置 Concat no-task 优化的完整校验链（如 32 字节对齐、前驱节点类型、级联 continuous input 等）。实际使用时需结合内存校验确保各 conv 输出地址满足连续性要求，否则可能导致运行结果错误。

## 目录结构<a name="section7668345634665"></a>

```
├── src
│   ├──conv_concat_notask_pass.cpp  // pass实现文件
├── CMakeLists.txt                  // 编译脚本
```

## 环境要求<a name="section383335652346"></a>

- 编译器：GCC >= 7.3.x
- 已完成[相关环境准备](../../../../docs/zh/build.md)。

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

    - 设置环境变量，dump出编译过程中的模型图：

      ```
      export DUMP_GE_GRAPH=1
      ```

    - 使用包含 Conv2D+Concat 结构的模型执行ATC命令，其中soc_version根据实际模型运行环境填写：

      **atc --model=./model.pb --framework=3 --soc_version=xxx --output=./model**

    - 检查执行结果：

      - 日志中出现如下打印：

        ```
        ConvConcatNotaskPass begin.
        ConvConcatNotaskPass end.
        ```

      - 对比NPU编译过程中间dump图（获取方法请参见CANN环境变量说明>编译相关>DUMP_GE_GRAPH），Concat节点上已携带`_no_task`等属性，各Conv2D节点上已携带`_output_offset_list_for_continuous`等属性。
