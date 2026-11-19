# --aicore\_num

## 产品支持情况

全量芯片支持

## 功能说明

用于配置模型编译时使用的AI Core核数。

## 参数取值

**参数值：**"整数1|整数2"（中间使用“|”分隔）或"整数1"，参数值的含义只取决于取值形式，以及AI处理器本身的Cube:Vector核数比例。整数1表示算子编译时使用的AI Core中的Cube Core核数。整数2表示算子编译时使用的Vector Core核数。

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

**参数值约束**：

- [--mode](./--mode.md)取值为7时，仅支持静态shape场景的核数控制。
- 成对写法中右侧配置为0（如"整数1|0"）或留空（如"整数1|"）时，等同于只配置整数1，会按单值写法参与Vector Core核数派生；只配置"0"（单值0）时等同于不配置该参数，不做派生。
- 整个参数值需大于等于0、小于等于对应核数的最大值；单值写法的取值范围为1~最大Cube Core核数（无独立Cube Core的产品为最大AI Core核数）。超出范围或格式非法（如带“+”、前导零、空格、小数、非数字、溢出）时，解析阶段仅打印告警、不派生，初始化阶段上报参数非法（E10001）。
- 如果配置该参数的同时启用了算子编译缓存功能（[--op\_compiler\_cache\_mode](--op_compiler_cache_mode.md)参数配置为“enable”或者“force”），此参数仅在首次编译时生效。若您想在非首次编译时生效该参数，需要清理编译磁盘的缓存。

其中，$\{INSTALL\_DIR\}请替换为CANN软件安装后文件存储路径。以root用户安装为例，安装后文件默认存储路径为：/usr/local/Ascend/cann。<arch\>表示具体操作系统架构，xxx请根据实际产品进行选择。

## 推荐配置及收益

无。

## 示例

- 场景1配置示例：

    ```bash
    atc --aicore_num="24|48" ...
    ```

- 场景2配置示例：

    ```bash
    atc --aicore_num="10|" ...
    或
    atc --aicore_num="10" ...
    ```

## 了解AI Core、Cube Core、Vector Core的关系

为便于理解AI Core、Cube Core、Vector Core的关系，此处先明确Core的定义，Core是指拥有独立Scalar计算单元的一个计算核，通常Scalar计算单元承担了一个计算核的SIMD（单指令多数据，Single Instruction Multiple Data）指令发射等功能，所以我们也通常也把这个Scalar计算单元称为核内的调度单元。不同产品上的AI数据处理核心单元不同，当前分为以下几类：

- 当AI数据处理核心单元是AI Core：
  - 在AI Core内，Cube和Vector共用一个Scalar调度单元。

    <!-- npu="910" id8 -->
    此处以Atlas训练系列产品为例。
    <!-- end id8 -->

    ![图示](../figures/logical_architecture.png)

  - 在AI Core内，Cube和Vector都有各自的Scalar调度单元，因此又被称为Cube Core、Vector Core。这时，一个Cube Core和一组Vector Core被定义为一个AI Core，AI Core数量通常是以多少个Cube Core为基准计算的。

    <!-- npu="910b" id9 -->
    此处以Atlas A2系列产品为例。
    <!-- end id9 -->

    ![图示](../figures/logical_architecture_0.png)

- 当AI数据处理核心单元是AI Core以及单独的Vector Core：AI Core和Vector Core都拥有独立的Scalar调度单元。

    <!-- npu="310p" id10 -->
    此处以Atlas推理系列产品为例。
    <!-- end id10 -->

    ![图示](../figures/logical_architecture_1.png)

## 依赖约束

无。
