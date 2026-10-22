# 试验参数

## BUILD\_INNER\_MODEL

当前版本暂不支持。

## OO\_CONSTANT\_FOLDING

**调试功能扩展参数，暂不支持应用于生产环境，后续版本会作为正式功能更新发布。**

是否开启常量折叠优化。此参数实际对应的options参数为`ge.oo.constantFolding`。

常量折叠是将计算图中可以预先确定输出值的节点替换成常量，并对计算图进行一些结构简化的操作。

**参数取值：**

- true：（默认值）开启。
- false：关闭。

**配置示例：**

```c++
{ge::ir_option::OO_CONSTANT_FOLDING, "true"}
```

**产品支持情况：**

全量芯片支持。

## OO\_DEAD\_CODE\_ELIMINATION

**调试功能扩展参数，暂不支持应用于生产环境，后续版本会作为正式功能更新发布。**

是否开启死边消除优化。此参数实际对应的options参数为`ge.oo.deadCodeElimination`。

死边消除：switch死边消除，switch的pred输入（1号输入）为const节点时，根据const的值消除其中一条分支：const为true时，消除false分支；const为false时，消除true分支。

**参数取值：**

- true：（默认值）开启。
- false：关闭。

**配置示例：**

```c++
{ge::ir_option::OO_DEAD_CODE_ELIMINATION, "true"}
```

**产品支持情况：**

全量芯片支持。

## OO\_LEVEL

**调试功能扩展参数，暂不支持应用于生产环境，后续版本会作为正式功能更新发布。**

图编译多级优化选项，包括子图优化、整图优化、静态Shape模型下沉等。此参数实际对应的options参数为`ge.oo.level`。

静态Shape模型下沉：静态Shape模型在编译时即可确定所有算子的输入输出Shape，完成模型级内存编排、算子的Tiling计算等Host侧计算，在模型加载时整体下发到Device流上，但不立即执行，通过下发模型执行Task触发模型中所有Task的执行。

**参数取值：**

- O1：会关闭所有图融合和UB融合Pass，只做促成静态下沉的相关优化，如InferShape（进行输出Tensor的shape推导）、常量折叠、死边消除等。

    Ascend 950PR&950DT系列产品不支持UB融合，只关闭图融合Pass。

- O3：（默认值）开启所有优化。

**使用约束：**

取值为O1时，会关闭所有图融合和UB融合PASS，只开启静态下沉的相关PASS，但是如下路径文件中的图融合PASS，由于关闭后会有功能问题，会默认开启：

`${INSTALL_DIR}/<arch>-linux/lib64/plugin/opskernel/fusion_pass/config/fusion_config.json`文件中"ExceptionalPassOfO1Level"字段下的所有图融合PASS。

其中$\{INSTALL\_DIR\}请替换为CANN软件安装后文件存储路径。以root用户安装为例，安装后文件默认存储路径为：/usr/local/Ascend/cann。<arch\>表示具体操作系统架构。

**配置示例：**

```c++
{ge::ir_option::OO_LEVEL, "O3"}
```

**产品支持情况：**

全量芯片支持。
