# 算子编译与图编译

## ge.aicoreNum

用于配置算子编译时使用的AI Core核数。

**参数取值：**"整数1|整数2"（中间使用“|”分隔）或"整数1"，参数值的含义只取决于取值形式，以及AI处理器本身的Cube:Vector核数比例。整数1表示算子编译时使用的AI Core中的Cube Core核数。整数2表示算子编译时使用的Vector Core核数。

- 取值场景1（成对写法）："整数1|整数2"形式，直接按配置分配，不做派生；整数1和整数2要满足1:1或者1:2关系。

- 取值场景2（单值）："整数1"，只显示配置AI Core核数，Vector Core核数由GE按AI处理器的Cube:Vector核数比例自动派生：
  - Cube:Vector比例为1:2时，Vector Core核数=2*整数1。
  - Cube:Vector比例为1:1时，Vector Core核数=整数1。

  "整数1"与"整数1|"、"整数1|0"等价，都会参与上述派生。

AI处理器的Cube:Vector核数比例可从`${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini`文件查看：

    ```ini
    [SoCInfo]
    # 参数配置为默认值，默认值即为最大值
    ai_core_cnt=24
    cube_core_cnt=24
    vector_core_cnt=48
    ```
如上所示，说明AI处理器上最大存在24个Cube Core，48个Vector Core，Cube:Vector核数比例为1:2，按照取值场景2，"整数1"配置为12时，派生24个Vector Core核数，若未配置`cube_core_cnt`，则以`ai_core_cnt`作为Cube Core数量计算比例。

**参数值约束：**

- 成对写法中右侧配置为0（如"整数1|0"）或留空（如"整数1|"）时，等同于只配置整数1，会按单值写法参与Vector Core核数派生；只配置"0"（单值0）时等同于不配置该参数，不做派生。
- 整个参数值需大于等于0、小于等于对应核数的最大值；单值写法的取值范围为1~最大Cube Core核数（无独立Cube Core的产品为最大AI Core核数）。超出范围或格式非法（如带“+”、前导零、空格、小数、非数字、溢出）时，解析阶段仅打印告警、不派生，初始化阶段上报参数非法（E10001）。
- 若同时通过[ge.vectorcoreNum](#gevectorcorenum)显示配置了正整数，则以显示值为准，不做派生；显示值与成对写法中的整数2冲突时，初始化阶段上报参数非法（E10001）。
- 如果配置该参数的同时启用了算子编译缓存功能（ge.op\_compiler\_cache\_mode参数配置为“enable”或者“force”，默认为“enable”），此参数仅在首次编译时生效。若您想在非首次编译时生效该参数，需要清理编译磁盘的缓存。

其中，$\{INSTALL\_DIR\}请替换为CANN软件安装后文件存储路径。以root用户安装为例，安装后文件默认存储路径为：/usr/local/Ascend/cann。<arch\>表示具体操作系统架构，xxx请根据实际产品进行选择。

**配置示例：**

- 场景1配置示例：

    ```c++
    {"ge.aicoreNum", "24|48"};
    ```

- 场景2配置示例

    ```c++
    {"ge.aicoreNum", "10|"};
    或
    {"ge.aicoreNum", "10"};
    ```

**必选/可选**：可选

**生效级别**：全局/session

**了解AI Core、Cube Core、Vector Core的关系**：

为便于理解AI Core、Cube Core、Vector Core的关系，此处先明确Core的定义，Core是指拥有独立Scalar计算单元的一个计算核，通常Scalar计算单元承担了一个计算核的SIMD（单指令多数据，Single Instruction Multiple Data）指令发射等功能，所以我们也通常也把这个Scalar计算单元称为核内的调度单元。不同产品上的AI数据处理核心单元不同，当前分为以下几类：

- 当AI数据处理核心单元是AI Core：
  - 在AI Core内，Cube和Vector共用一个Scalar调度单元.
    <!-- npu="910" id12 -->
    例如Atlas训练系列产品
    <!-- end id12 -->

    ![图示](../../../figures/logic_arch_diagram.png)

  - 在AI Core内，Cube和Vector都有各自的Scalar调度单元，因此又被称为Cube Core、Vector Core。这时，一个Cube Core和一组Vector Core被定义为一个AI Core，AI Core数量通常是以多少个Cube Core为基准计算的。
    <!-- npu="910b" id13 -->
    例如Atlas A2系列产品。
    <!-- end id13 -->

    ![图示](../../../figures/logic_arch_diagram_0.png)

- 当AI数据处理核心单元是AI Core以及单独的Vector Core：AI Core和Vector Core都拥有独立的Scalar调度单元。
    <!-- npu="310p" id14 -->
    例如Atlas推理系列产品。
    <!-- end id14 -->

    ![图示](../../../figures/logic_arch_diagram_1.png)

## ge.AllowMultiGraphParallelCompile

用于控制同一个Session内多线程并行编译多张图的能力。如果配置为"1"，会禁止变量转换格式（参考ge.exec.variable\_acc说明）。

**参数取值：**

- "0"，默认值，同一个Session内一个线程编译多张图。
- "1"，表示允许用户在一个Session内使用多个线程并发编译多张图。

**使用约束：**

- 如果配置为"1"，不能同时配置ge.exec.variable\_acc为"True"，否则会校验报错。
- 如果配置为"1"，资源类算子如果导致其他图重编译，立即报错。

**配置示例：**

```c++
{"ge.AllowMultiGraphParallelCompile", "1"};
```

**必选/可选**：可选

**生效级别**：全局/session

## ge.compile.h2dOverlappedWithCompute

用于配置模型输入的H2D拷贝是否与模型计算重叠执行。

该参数作为图编译选项传递给GE，GE首先筛选可以参与该功能的输入，并按照输入首次被使用的位置排序：

- 首次消费节点的拓扑序越小，排序越靠前。
- 首次消费节点相同时，按input index升序排列。

排序后，每个输入最初对应一个独立的H2D拷贝。接下来，GE将相邻输入合并为若干拷贝组。同一组内的输入会通过一条H2D拷贝指令批量拷贝到Device。

第一组在模型开始计算前完成拷贝；其余组在模型计算过程中拷贝，与模型计算重叠执行。

**参数取值：**

- "0"：关闭重叠。所有输入完成H2D拷贝后，模型才开始计算。默认值。
- "1"：开启自动规划。GE自动对输入分组，并在模型计算过程中拷贝部分输入。
- 逗号分隔的分组边界，例如"44,64,68"：手动指定输入如何分组。

    手动配置的数字表示排序后输入序列的分组边界。例如："44,64,68"，表示将输入分为四个拷贝组：[0,44)、[44,64)、[64,68)、[68,N)，其中，[0,44)在模型开始计算前完成拷贝，其余三组在模型计算过程中执行拷贝。

    如果只需要一个边界，可以配置为：`44,`或`,44`

    两种写法均表示[0,44)在模型开始计算前完成拷贝，[44,N)在模型计算过程中执行拷贝。

    需要注意的是，**边界编号基于GE对候选输入的排序结果，不是模型的input index**。GE会通过INFO日志打印最终采用的分组边界和拷贝区间：

    ```text
    [InputH2DOverlap] H2D overlap groups: mode=auto, boundaries=[44,64,68]
    ```

    手动配置时，mode显示为manual：

    ```text
    [InputH2DOverlap] H2D overlap groups: mode=manual, boundaries=[44,64,68]
    ```

**推荐配置：**

适用于静态shape、单流图，并且模型包含较多Host输入、H2D拷贝开销较大、模型前段计算能够覆盖部分输入拷贝场景。

建议先配置为"1"，由GE自动规划，并根据上文INFO日志查看实际分组结果。

如果profiling trace中显示模型计算存在明显的H2D等待，说明被等待的拷贝组过长，此时可手动调整分组边界，减少本组的拷贝数量。

> 注：GE INFO日志通过环境变量`ASCEND_GLOBAL_LOG_LEVEL`设置，取值为`1`表示INFO级别（0=DEBUG，1=INFO，2=WARN，3=ERROR），详细说明请参见《[环境变量参考](https://gitcode.com/cann/docs/blob/master/docs/en/env-vars/README.md)》。

**配置示例：**

- 自动规划：

    ```c++
    {"ge.compile.h2dOverlappedWithCompute", "1"};
    ```

- 手动分组：

    ```c++
    {"ge.compile.h2dOverlappedWithCompute", "44,64,68"};
    ```

**使用约束：**

- 当前仅对静态shape模型生效。
- 当前仅支持单流图。
- 手动配置的边界必须是非负整数，并按升序排列。
- 手动配置的边界必须以当前模型日志打印的输入排序和分组结果为准。
- 模型结构或拓扑排序发生变化后，需要重新确认分组边界。

**必选/可选**：可选

**生效级别**：全局/session/graph

## ge.graph\_compiler\_cache\_dir

图编译磁盘缓存目录，和ge.graph\_key配合使用，ge.graph\_compiler\_cache\_dir和ge.graph\_key同时配置非空时图编译磁盘缓存功能生效。

配置的缓存目录必须存在，否则会导致编译失败。

图发生变化后，原来的缓存文件不可用，用户需要手动删除缓存目录中的缓存文件或者修改ge.graph\_key，重新编译生成缓存文件。

其他使用约束和具体的使用方法请参见[图编译缓存](../../../../../user_guides/graph_dev/more_features/graph_compile_cache.md)。

**必选/可选**：可选

**生效级别**：session

## ge.graph\_key

图唯一标识，取值只包含大小写字母（A-Z，a-z）、数字（0-9）、下划线（\_）、中划线（-）并且长度不超过128。

**必选/可选**：可选

**生效级别**：graph

## ge.op\_compiler\_cache\_mode

用于配置算子编译磁盘缓存模式。

**参数取值：**

- enable：（默认值）表示启用算子编译缓存。启用后可以避免针对相同编译参数及算子参数的算子重复编译，从而提升编译速度。
- force：启用算子编译缓存功能，区别于enable模式，force模式下会强制刷新缓存，即先删除已有缓存，再重新编译并加入缓存。比如当用户的python变更、依赖库变更、算子调优后知识库变更等，需要先指定为force用于先清理已有的缓存，后续再修改为enable模式，以避免每次编译时都强制刷新缓存。
- disable：表示禁用算子编译缓存，算子重新编译。

**使用约束：**

- 如果要自行指定算子编译磁盘缓存的路径，则需要与ge.op\_compiler\_cache\_dir配合使用。
- 由于force选项会先删除已有缓存，所以不建议在程序并行编译时设置，否则可能会导致其他模型使用的缓存内容被清除而导致失败。
- 建议模型最终发布时设置编译缓存选项为disable或者force。
- 如果算子调优后知识库变更，则需要先通过设置为force来清空缓存，然后再设置为enable重新进行编译，否则无法应用新的调优知识库，从而导致调优应用执行失败。
- 调试开关打开的场景下：
  - ge.opDebugLevel非0值：会忽略ge.op\_compiler\_cache\_mode参数的配置，不启用算子编译缓存功能，算子全部重新编译。
  - op\_debug\_config配置非空时，且**未配置op\_debug\_list字段**，会忽略ge.op\_compiler\_cache\_mode参数的配置，不启用算子编译缓存功能，算子全部重新编译。
  - op\_debug\_config配置非空时，且**配置文件中配置了op\_debug\_list字段**：
    - 列表中的算子，忽略ge.op\_compiler\_cache\_mode参数的配置继续重新编译。
    - 列表外的算子，如果ge.op\_compiler\_cache\_mode参数配置为enable或force，则启用缓存功能；若配置为disable，则不启用缓存功能，仍旧重新编译。

- 启用算子编译缓存功能时，可以通过**配置文件**（算子编译完成后，会在ge.op\_compiler\_cache\_dir参数指定路径下自动生成op\_cache.ini文件）、**环境变量**两种方式来设置缓存文件夹的磁盘空间大小：

    1. 通过配置文件op\_cache.ini设置

        若op\_cache.ini文件不存在，则需要手动创建。打开该文件，增加如下信息：

        ```ini
        #配置文件格式，必须包含，自动生成的文件中默认包括如下信息，手动创建时，需要输入
        [op_compiler_cache]
        #限制某个芯片下缓存文件夹的磁盘空间的大小，默认值为500，取值需为整数，单位为MB
        max_op_cache_size=500
        #设置需要保留缓存的空间大小比例，取值范围：[1,100]，默认值为50，单位为百分比；例如取值为80表示缓存空间不足时，删除缓存文件，保留80%缓存空间
        remain_cache_size_ratio=50
        ```

        - 上述文件中的max\_op\_cache\_size和remain\_cache\_size\_ratio参数取值都有效时，op\_cache.ini文件才会生效。
        - 当编译缓存文件大小超过“max\_op\_cache\_size”的设置值，且超过半小时缓存文件未被访问时，缓存文件就会老化（算子编译时，不会因为编译缓存文件大小超过设置值而中断，所以当“max\_op\_cache\_size”设置过小时，会出现实际编译缓存文件大小超过此设置值的情况）。
        - 若需要关闭编译缓存老化功能，可将“max\_op\_cache\_size”设置为“-1”，此时访问算子缓存时不会更新访问时间，算子编译缓存不会老化，磁盘空间使用默认大小500MB。
        - 若多个使用者使用相同的缓存路径，建议使用配置文件的方式进行设置，该场景下op\_cache.ini文件会影响所有使用者。

    2. 通过环境变量设置

        该场景下，开发者可以通过环境变量ASCEND\_MAX\_OP\_CACHE\_SIZE来限制某个芯片下缓存文件夹的磁盘空间的大小，当编译缓存空间大小达到ASCEND\_MAX\_OP\_CACHE\_SIZE设置的取值，且超过半个小时缓存文件未被访问时，缓存文件就会老化。可通过环境变量ASCEND\_REMAIN\_CACHE\_SIZE\_RATIO设置需要保留缓存的空间大小比例。

        配置示例如下：

        ```bash
        # ASCEND_MAX_OP_CACHE_SIZE环境变量默认值为500，取值需为整数，单位为MB
        export ASCEND_MAX_OP_CACHE_SIZE=500
        # ASCEND_REMAIN_CACHE_SIZE_RATIO环境变量取值范围：[1,100]，默认值为50，单位为百分比；例如取值为80表示缓存空间不足时，删除缓存文件，保留80%缓存空间
        export ASCEND_REMAIN_CACHE_SIZE_RATIO=50
        ```

        - 通过环境变量配置，只对当前用户生效。
        - 若需要关闭编译缓存老化功能，可将环境变量“**ASCEND\_MAX\_OP\_CACHE\_SIZE**”设置为“-1”，此时访问算子缓存时不会更新访问时间，算子编译缓存不会老化，磁盘空间使用默认大小500MB。

    **若同时配置了op\_cache.ini文件和环境变量，则优先读取op\_cache.ini文件中的配置项，若op\_cache.ini文件和环境变量都未设置，则读取系统默认值：默认磁盘空间大小500MB，默认保留缓存空间的50%。**

**配置示例：**

```c++
{"ge.op_compiler_cache_mode", "enable"};
```

**必选/可选**：可选

**生效级别**：全局/session/graph

## ge.op\_compiler\_cache\_dir

用于配置算子编译磁盘缓存的目录。

**参数值格式**：路径支持大小写字母（a-z，A-Z）、数字（0-9）、下划线（\_）、中划线（-）、句点（.）、中文字符。

**默认值：**$HOME/atc\_data

- 如果参数指定的路径存在且有效，则在指定的路径下自动创建子目录kernel\_cache；如果指定的路径不存在但路径有效，则先自动创建目录，然后在该路径下自动创建子目录kernel\_cache。
- 用户请不要在**默认缓存目录**下存放其他自有内容，自有内容在软件包安装或升级时会同默认缓存目录一并被删除。
- 通过该参数指定的**非默认缓存目录**无法删除（软件包安装或升级时不会被删除）。
- 算子编译磁盘缓存路径，除ge.op\_compiler\_cache\_dir参数设置的方式外，还可以配置环境变量ASCEND\_CACHE\_PATH，几种方式优先级为：配置参数“ge.op\_compiler\_cache\_dir”\>环境变量ASCEND\_CACHE\_PATH\>默认存储路径。关于环境变量ASCEND\_CACHE\_PATH的详细说明请参见《[环境变量参考](https://gitcode.com/cann/docs/blob/master/docs/zh/env-vars/README.md)》。

**必选/可选**：可选

**生效级别**：全局/session/graph

## ge.optimizationSwitch

算子编译时，融合规则（Pass）的控制开关。

该参数与ge.fusionSwitchFile的区别是：ge.fusionSwitchFile仅能关闭图融合和UB融合的规则，并且需要单独配置JSON文件，而该参数适用于所有规则，通过参数就能指定融合规则，不需要再单独设置JSON文件。如果两个参数都配置，且配置了同一个融合规则，则以ge.optimizationSwitch参数配置的为准。

**参数取值：**"Passname1:on;Passname2:off"，可以拼接多个key-value键值对，key为Pass名称，value为on（表示开）或off（表示关），不支持大小写模式匹配，多组配置使用英文分号分隔。可配置的融合规则请参见[融合规则列表](../../../../../user_guides/graph_dev/appendix/fusion_rule_list.md)。

**配置示例：**

```c++
{"ge.optimizationSwitch", "Passname1:on;Passname2:off"};
```

**必选/可选**：可选

**生效级别**：全局/session/graph

## ge.topoSortingMode

对算子进行图模式编译时，可选择的不同的图遍历模式。主要面向在线推理场景。

**参数取值：**

- 0：BFS，Breadth First Search，广度优先遍历策略。
- 1：（默认值）DFS，Depth First Search，深度优先遍历策略。
- 2：RDFS，Reverse DFS，反向深度优先遍历策略。
- 3：StableRDFS，稳定拓扑序策略，针对图里已有的算子，不会改变其计算顺序；针对图里新增的算子，使用RDFS遍历策略。

**配置示例：**

```c++
{"ge.topoSortingMode", "1"};
```

**必选/可选**：可选

**生效级别**：全局/session/graph

## ge.vectorcoreNum

用于配置算子编译时使用的Vector Core核数。

与[ge.aicoreNum](#geaicorenum)配套使用：ge.aicoreNum支持“AI Core数\|Vector Core数”成对写法；当ge.aicoreNum仅配置单值（或成对写法右侧为空、0）时，GE按AI处理器Cube:Vector比例自动派生本参数并写回options；显式配置本参数时优先生效，ge.aicoreNum不再触发派生覆盖。

**参数取值：**

- 正整数n：1~AI处理器包含的最大Vector Core核数（`${INSTALL_DIR}/<arch>-linux/data/platform_config/xxx.ini`文件中的`vector_core_cnt`取值）。
- 0或空：表示未配置，按默认的`vector_core_cnt`生效。

**参数值约束：**

- 仅支持非负十进制数字字符串，不支持前导零、空格、正负号、小数点等；格式非法报E10001。
- 显式配置的取值优先于ge.aicoreNum派生：ge.aicoreNum单值写法（含"6\|"、"6\|0"）不再派生覆盖；ge.aicoreNum成对写法右侧为空或0时保留本参数显式值。
- ge.aicoreNum成对写法中的Vector数与本参数均配置为正整数且不相等时，报E10001（PARAM_INVALID）。
- 自动派生仅在AI处理器Cube:Vector比例为1:1或1:2时进行，比例为1:1时派生值等于AI Core数，为1:2时派生值为AI Core数的2倍；比例不支持、AI处理器无独立Vector Core、AI Core数超过Cube核数时不派生，仅打印告警信息。
- 核数生效优先级：算子级属性`_op_vectorcore_num` > 本参数 > 默认`vector_core_cnt`。

**配置示例：**

```c++
{"ge.vectorcoreNum", "12"};
```

**必选/可选**：可选

**生效级别**：全局/session
